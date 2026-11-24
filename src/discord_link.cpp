#include "discord_link.hpp"
#include "auth.hpp"
#include "auth_step.hpp"
#include "deafen_intent.hpp"
#include "discord_socket.hpp"
#include "socket_paths.hpp"

#include <Geode/Geode.hpp>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>

using namespace geode::prelude;
using namespace std::chrono_literals;

// Never log payloads here: they carry tokens. Error codes are fine.
namespace {
    enum Op : uint32_t { HANDSHAKE = 0, FRAME = 1, CLOSE = 2, PING = 3, PONG = 4 };
    constexpr auto REPLY_WAIT = 5000ms;
    constexpr auto APPROVAL_WAIT = 120000ms;
    constexpr auto RETRY_WAIT = 5000ms;
    constexpr auto POLL = 50ms;
    constexpr int INVALID_CLIENT_ID = 4000;

    struct Shared {
        std::mutex mutex;
        std::condition_variable changed;
        bool started = false, stopping = false, done = false;
        // Stays set until Discord is asked, so it survives Discord being closed.
        bool authorize = false;
        // Cuts a wait short once.
        bool poke = false;
        bool loginFailed = false;
        bool wanted = false;
        LinkStatus status = LinkStatus::Disconnected;
    };
    // Never freed, so the thread can outlive static destruction at exit.
    Shared& shared() {
        static auto* s_shared = new Shared;
        return *s_shared;
    }

    std::optional<std::filesystem::path> prepareHelper() {
        static auto const s_script = [] {
            auto script = helper_link::install(
                Mod::get()->getResourcesDir().native(), Mod::get()->getSaveDir().native()
            );
            if (script) log::info("Using the Linux helper to reach Discord");
            else log::error("Could not install the Linux helper");
            return script;
        }();
        return s_script;
    }

    class Worker {
        DiscordSocket m_socket;
        DeafenIntent m_intent;
        unsigned m_nonce = 0;
        // The app ID Discord said it does not know.
        std::string m_badApp;
        int m_closeCode = 0;

        static void setStatus(LinkStatus status) {
            std::lock_guard lock(shared().mutex);
            shared().status = status;
        }
        static bool stopping() {
            std::lock_guard lock(shared().mutex);
            return shared().stopping;
        }
        static bool interrupted() {
            std::lock_guard lock(shared().mutex);
            return shared().stopping || shared().authorize;
        }
        template <class Pred>
        static void waitFor(std::chrono::milliseconds time, Pred until) {
            std::unique_lock lock(shared().mutex);
            shared().changed.wait_for(lock, time, until);
        }
        static void pause(std::chrono::milliseconds time = RETRY_WAIT) {
            waitFor(time, [] { return shared().stopping || shared().poke; });
            std::lock_guard lock(shared().mutex);
            shared().poke = false;
        }
        static bool takeAuthorize() {
            std::lock_guard lock(shared().mutex);
            shared().poke = false;
            return std::exchange(shared().authorize, false);
        }
        static bool authorizePending() {
            std::lock_guard lock(shared().mutex);
            return shared().authorize;
        }

        // One message from Discord. Answers pings itself.
        std::optional<matjson::Value> read(std::chrono::milliseconds wait) {
            auto message = m_socket.receive(wait);
            if (!message) return {};
            auto& [op, body] = *message;
            if (op == PING) m_socket.send(PONG, body);
            if (op == CLOSE) {
                auto why = matjson::parse(body).unwrapOrDefault();
                m_closeCode = static_cast<int>(why["code"].asInt().unwrapOr(0));
                m_socket.close();
            }
            if (op != FRAME) return {};
            auto json = matjson::parse(body);
            if (!json) return {};
            auto value = std::move(json).unwrap();
            if (value["evt"].asString().unwrapOrDefault() == "VOICE_SETTINGS_UPDATE") {
                if (auto deaf = value["data"]["deaf"].asBool()) m_intent.observed(deaf.unwrap());
            }
            return value;
        }

        bool handshake(std::string const& appId) {
            auto hello = matjson::makeObject({ { "v", 1 }, { "client_id", appId } });
            if (!m_socket.send(HANDSHAKE, hello.dump(matjson::NO_INDENTATION))) return false;
            auto deadline = DiscordSocket::Clock::now() + REPLY_WAIT;
            while (m_socket.isOpen() && DiscordSocket::Clock::now() < deadline && !stopping()) {
                auto json = this->read(POLL);
                if (json && (*json)["evt"].asString().unwrapOrDefault() == "READY") return true;
            }
            return false;
        }

        // Tells us when the player deafens or undeafens themself. Not essential.
        void subscribe() {
            auto nonce = std::to_string(++m_nonce);
            auto json = matjson::makeObject({
                { "cmd", "SUBSCRIBE" }, { "evt", "VOICE_SETTINGS_UPDATE" },
                { "nonce", nonce }, { "args", matjson::Value::object() },
            });
            m_socket.send(FRAME, json.dump(matjson::NO_INDENTATION));
        }

        struct Reply {
            bool answered = false;
            bool ok = false;
            matjson::Value data;
        };

        Reply request(
            char const* cmd, matjson::Value args, std::chrono::milliseconds wait,
            bool evenIfStopping = false, bool untilAuthorize = false
        ) {
            auto nonce = std::to_string(++m_nonce);
            auto json = matjson::makeObject({ { "cmd", cmd }, { "nonce", nonce }, { "args", std::move(args) } });
            if (!m_socket.send(FRAME, json.dump(matjson::NO_INDENTATION))) return {};

            auto deadline = DiscordSocket::Clock::now() + wait;
            while (m_socket.isOpen() && DiscordSocket::Clock::now() < deadline) {
                if (!evenIfStopping && stopping()) return {};
                // Pressed Authorize again: this popup no longer counts.
                if (untilAuthorize && authorizePending()) return {};
                auto reply = this->read(POLL);
                if (!reply || (*reply)["nonce"].asString().unwrapOrDefault() != nonce) continue;
                bool ok = (*reply)["evt"].asString().unwrapOrDefault() != "ERROR";
                if (!ok) {
                    log::warn("Discord refused {} with code {}", cmd, (*reply)["data"]["code"].asInt().unwrapOr(0));
                }
                return { true, ok, (*reply)["data"] };
            }
            return {};
        }

        bool askForApproval(std::string const& appId) {
            setStatus(LinkStatus::WaitingForApproval);
            std::vector<matjson::Value> scopes { "rpc", "rpc.voice.read", "rpc.voice.write" };
            auto reply = this->request(
                "AUTHORIZE", matjson::makeObject({ { "client_id", appId }, { "scopes", scopes } }), APPROVAL_WAIT, false, true
            );
            auto code = reply.data["code"].asString().unwrapOrDefault();
            return reply.ok && !code.empty() && !stopping() && auth::exchange(code) == auth::Result::Ok;
        }

        enum class LoginResult { Ok, NeedSetup, TryLater };

        LoginResult login() {
            bool rejected = false, refreshFailed = false;
            while (true) {
                auto saved = auth::current();
                switch (nextAuthStep(!saved.token.empty(), !saved.refresh.empty(), rejected, refreshFailed)) {
                    case AuthStep::UseToken: {
                        auto reply = this->request(
                            "AUTHENTICATE", matjson::makeObject({ { "access_token", saved.token } }), REPLY_WAIT
                        );
                        if (reply.ok) return LoginResult::Ok;
                        if (!reply.answered) return LoginResult::TryLater;
                        rejected = true;
                        break;
                    }
                    case AuthStep::Refresh:
                        if (stopping()) return LoginResult::TryLater;
                        switch (auth::refresh()) {
                            case auth::Result::Ok:
                                rejected = false;
                                break;
                            case auth::Result::Rejected:
                                break;
                            case auth::Result::NoNetwork:
                                // Do not hammer Discord's login server.
                                m_socket.close();
                                setStatus(LinkStatus::Disconnected);
                                pause(30000ms);
                                return LoginResult::TryLater;
                        }
                        // Refresh once only, whether it worked or not.
                        refreshFailed = true;
                        break;
                    case AuthStep::NeedSetup:
                        return LoginResult::NeedSetup;
                }
            }
        }

        // False if Discord did not answer, so the caller backs off.
        bool applyIntent(std::chrono::milliseconds wait, bool evenIfStopping) {
            auto settings = this->request("GET_VOICE_SETTINGS", matjson::Value::object(), wait, evenIfStopping);
            auto deaf = settings.data["deaf"].asBool();
            if (!settings.ok || !deaf) return false;
            auto send = m_intent.next(deaf.unwrap());
            if (!send) return true;
            auto reply = this->request(
                "SET_VOICE_SETTINGS", matjson::makeObject({ { "deaf", *send } }), wait, evenIfStopping
            );
            if (reply.ok) m_intent.applied(*send);
            else if (reply.answered) m_intent.refused(*send);
            return reply.ok;
        }

        void serve() {
            int failures = 0;
            while (m_socket.isOpen() && !interrupted()) {
                {
                    std::lock_guard lock(shared().mutex);
                    m_intent.want(shared().wanted);
                }
                if (!m_intent.needsCheck()) {
                    if (std::exchange(failures, 0)) setStatus(LinkStatus::Connected);
                    this->read(POLL);
                }
                else if (this->applyIntent(REPLY_WAIT, false)) {
                    if (failures) setStatus(LinkStatus::Connected);
                    failures = 0;
                }
                else {
                    setStatus(LinkStatus::Refused);
                    // Start over from login: the login may have been revoked. Not
                    // while the player is owed an undeafen, which must not be forgotten.
                    if (++failures >= 3 && !m_intent.owesUndeafen()) break;
                    waitFor(1000ms, [] { return shared().stopping || shared().authorize; });
                }
            }
            // Leaving on purpose: do not leave the player deafened.
            m_intent.release();
            if (m_socket.isOpen() && m_intent.needsCheck()) {
                this->applyIntent(stopping() ? 150ms : REPLY_WAIT, true);
            }
        }

        void session() {
            auto login = auth::current();
            bool authorize, failed;
            {
                std::lock_guard lock(shared().mutex);
                authorize = shared().authorize;
                if (authorize) shared().loginFailed = false;
                failed = shared().loginFailed;
            }
            bool hasLogin = !login.token.empty() || !login.refresh.empty();
            if (!login.hasApp() || failed || (!authorize && !hasLogin)) {
                setStatus(LinkStatus::NeedSetup);
                return pause();
            }

            if (!authorize && login.appId == m_badApp) {
                setStatus(LinkStatus::BadApp);
                return pause();
            }

            auto dir = runtimeDir(std::getenv("WINE_HOST_XDG_RUNTIME_DIR"), std::getenv("XDG_RUNTIME_DIR"));
            m_closeCode = 0;
            switch (m_socket.connect(dir, { prepareHelper, stopping })) {
                case DiscordSocket::Result::Unsupported:
                    setStatus(LinkStatus::Unsupported);
                    return pause(15000ms);
                case DiscordSocket::Result::NotRunning:
                    setStatus(LinkStatus::Disconnected);
                    // Starting the helper is heavier than trying a socket.
                    return pause(m_socket.usesHelper() ? 15000ms : RETRY_WAIT);
                case DiscordSocket::Result::Connected:
                    break;
            }

            if (!this->handshake(login.appId)) {
                bool badApp = m_closeCode == INVALID_CLIENT_ID;
                if (badApp && auth::current().appId == login.appId) {
                    takeAuthorize();
                    m_badApp = login.appId;
                }
                setStatus(badApp ? LinkStatus::BadApp : LinkStatus::Disconnected);
                m_socket.close();
                return pause();
            }
            m_badApp.clear();
            // A different app was entered while connecting: start over with it.
            if (auto now = auth::current(); now.appId != login.appId || now.secret != login.secret) {
                m_socket.close();
                return;
            }
            // Saying no to the popup keeps a login that still works.
            if (takeAuthorize() && !this->askForApproval(login.appId) && !hasLogin) {
                setStatus(LinkStatus::NeedSetup);
                m_socket.close();
                return pause();
            }

            auto result = this->login();
            if (result != LoginResult::Ok) {
                if (result == LoginResult::NeedSetup) {
                    std::lock_guard lock(shared().mutex);
                    shared().loginFailed = true;
                }
                setStatus(result == LoginResult::NeedSetup ? LinkStatus::NeedSetup : LinkStatus::Disconnected);
                m_socket.close();
                return pause();
            }

            log::info("Connected to Discord");
            this->subscribe();
            setStatus(LinkStatus::Connected);
            this->serve();
            m_socket.close();
            m_intent.linkLost();
            setStatus(LinkStatus::Disconnected);
            if (!interrupted()) pause();
        }

    public:
        void run() {
            while (!stopping()) {
                // An exception leaving a thread would crash the game.
                try {
                    this->session();
                }
                catch (std::exception const&) {
                    log::error("Discord link failed, starting over");
                    m_socket.close();
                    m_intent.linkLost();
                    setStatus(LinkStatus::Disconnected);
                    pause();
                }
            }
            m_socket.close();
            std::lock_guard lock(shared().mutex);
            shared().done = true;
            shared().changed.notify_all();
        }
    };
}

void link::start() {
    std::lock_guard lock(shared().mutex);
    if (std::exchange(shared().started, true)) return;
    std::thread([] { Worker().run(); }).detach();
}

void link::stop() {
    std::unique_lock lock(shared().mutex);
    if (!shared().started) return;
    shared().stopping = true;
    shared().changed.notify_all();
    shared().changed.wait_for(lock, 500ms, [] { return shared().done; });
}

void link::want(bool deaf) {
    std::lock_guard lock(shared().mutex);
    shared().wanted = deaf;
}

void link::authorize() {
    std::lock_guard lock(shared().mutex);
    shared().authorize = true;
    shared().poke = true;
    shared().changed.notify_all();
}

LinkStatus link::status() {
    std::lock_guard lock(shared().mutex);
    return shared().status;
}
