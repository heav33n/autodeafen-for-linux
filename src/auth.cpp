#include "auth.hpp"
#include "auth_step.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <mutex>

using namespace geode::prelude;

// Never log anything from this file except status codes: it handles secrets.
namespace {
    std::mutex s_mutex;
    auth::Login s_login;

    void save() {
        // Reads the login when it runs, so an older save can't undo a newer one.
        queueInMainThread([] {
            auto login = auth::current();
            auto mod = Mod::get();
            mod->setSavedValue("app-id", login.appId);
            mod->setSavedValue("app-secret", login.secret);
            mod->setSavedValue("token", login.token);
            mod->setSavedValue("refresh", login.refresh);
            if (auto res = mod->saveData(); !res) log::warn("Could not save the login");
        });
    }

    std::string escape(std::string_view text) {
        std::string out;
        for (unsigned char c : text) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out.push_back(c);
            else out += fmt::format("%{:02X}", c);
        }
        return out;
    }

    auth::Result request(auth::Login const& used, std::string const& grant) {
        auto body = fmt::format(
            "client_id={}&client_secret={}&{}", escape(used.appId), escape(used.secret), grant
        );
        web::WebRequest req;
        req.header("Content-Type", "application/x-www-form-urlencoded");
        req.timeout(asp::Duration::fromSecs(5));
        req.bodyString(body);
        auto res = req.postSync("https://discord.com/api/oauth2/token");

        if (!res.ok()) {
            log::warn("Discord login request failed with code {}", res.code());
            auto error = res.json().unwrapOrDefault()["error"].asString().unwrapOrDefault();
            return isRefusal(res.code(), error) ? auth::Result::Rejected : auth::Result::NoNetwork;
        }
        auto json = res.json().unwrapOrDefault();
        auto token = json["access_token"].asString().unwrapOrDefault();
        if (token.empty()) return auth::Result::NoNetwork;

        {
            std::lock_guard lock(s_mutex);
            // The user may have entered another app while this was in flight.
            if (s_login.appId != used.appId || s_login.secret != used.secret) {
                return auth::Result::NoNetwork;
            }
            s_login.token = std::move(token);
            s_login.refresh = json["refresh_token"].asString().unwrapOr(std::string(s_login.refresh));
        }
        save();
        return auth::Result::Ok;
    }
}

void auth::load() {
    auto mod = Mod::get();
    std::lock_guard lock(s_mutex);
    s_login = {
        mod->getSavedValue<std::string>("app-id"),
        mod->getSavedValue<std::string>("app-secret"),
        mod->getSavedValue<std::string>("token"),
        mod->getSavedValue<std::string>("refresh"),
    };
}

auth::Login auth::current() {
    std::lock_guard lock(s_mutex);
    return s_login;
}

void auth::setApp(std::string id, std::string secret) {
    {
        std::lock_guard lock(s_mutex);
        s_login = { std::move(id), std::move(secret), "", "" };
    }
    save();
}

auth::Result auth::exchange(std::string const& code) {
    return request(current(), fmt::format(
        "grant_type=authorization_code&code={}&redirect_uri={}", escape(code), escape("http://localhost")
    ));
}

auth::Result auth::refresh() {
    auto login = current();
    return request(login, "grant_type=refresh_token&refresh_token=" + escape(login.refresh));
}
