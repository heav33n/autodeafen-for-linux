#include "helper_link.hpp"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>

#include <chrono>
#include <fstream>
#include <random>
#include <utility>

namespace {
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    constexpr size_t NONCE_LENGTH = 32;
    constexpr auto START_WAIT = 5000ms;
    constexpr auto REPLY_WAIT = 2000ms;

    class Socket {
        SOCKET m_socket;

    public:
        explicit Socket(SOCKET socket) : m_socket(socket) {}
        Socket(Socket const&) = delete;
        Socket& operator=(Socket const&) = delete;
        ~Socket() {
            if (m_socket != INVALID_SOCKET) closesocket(m_socket);
        }
        SOCKET get() const { return m_socket; }
        SOCKET release() { return std::exchange(m_socket, INVALID_SOCKET); }
        bool valid() const { return m_socket != INVALID_SOCKET; }
    };

    std::string makeNonce() {
        constexpr char DIGITS[] = "0123456789abcdef";
        std::random_device random;
        std::string nonce;
        while (nonce.size() < NONCE_LENGTH) nonce.push_back(DIGITS[random() & 15]);
        return nonce;
    }

    // Waits in short steps so quitting the game is never held up.
    bool readable(SOCKET socket, Clock::time_point deadline, std::function<bool()> const& cancelled) {
        while (!cancelled()) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(socket, &set);
            timeval step { 0, 50000 };
            auto ready = select(0, &set, nullptr, nullptr, &step);
            if (ready != 0) return ready > 0;
            if (Clock::now() >= deadline) return false;
        }
        return false;
    }

    std::optional<char> readByte(
        SOCKET socket, Clock::time_point deadline, std::function<bool()> const& cancelled
    ) {
        char byte;
        if (!readable(socket, deadline, cancelled) || recv(socket, &byte, 1, 0) != 1) return {};
        return byte;
    }

    // The helper writes its own port here. Only this user can, so a match means
    // the connection really is the helper's.
    bool proven(std::filesystem::path const& proof, unsigned short port) {
        std::ifstream file(proof);
        unsigned written = 0;
        return file >> written && written == port;
    }

    std::wstring widen(std::string const& text) {
        return std::filesystem::path(reinterpret_cast<char8_t const*>(text.c_str())).wstring();
    }

    bool start(std::filesystem::path const& script, std::string const& arguments) {
        auto command = L"\"" + script.wstring() + L"\" " + widen(arguments);
        STARTUPINFOW startup {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process {};
        if (!CreateProcessW(
            nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process
        )) return false;
        if (process.hThread) CloseHandle(process.hThread);
        if (process.hProcess) CloseHandle(process.hProcess);
        return true;
    }

    // Removes the proof however connecting ends.
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    };
}

std::optional<std::filesystem::path> helper_link::install(
    std::filesystem::path const& from, std::filesystem::path const& to
) {
    for (auto name : { "autodeafen-helper", "autodeafen-start" }) {
        auto target = to / (std::string(name) + ".exe");
        std::error_code error;
        std::filesystem::copy_file(from / name, target, std::filesystem::copy_options::overwrite_existing, error);
        // Copying over a helper that is still running fails, and it is fine.
        if (error && !std::filesystem::exists(target, error)) return {};
    }
    return to / "autodeafen-start.exe";
}

helper_link::Result helper_link::connect(
    std::filesystem::path const& script, std::string const& runtimeDir,
    std::function<bool()> const& cancelled, uintptr_t& socket
) {
    Socket listener(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!listener.valid()) return Result::Failed;
    sockaddr_in address {};
    address.sin_family = AF_INET;
    // This PC only.
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int size = sizeof(address);
    if (bind(listener.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) return Result::Failed;
    if (listen(listener.get(), 4) != 0) return Result::Failed;
    if (getsockname(listener.get(), reinterpret_cast<sockaddr*>(&address), &size) != 0) return Result::Failed;

    auto nonce = makeNonce();
    Cleanup proof { script.parent_path() / ("autodeafen-" + nonce) };
    // Nothing here is secret: other users can read command lines. A folder with
    // quotes in it would break out of its argument, so let the helper find it.
    auto dir = runtimeDir.find('"') == std::string::npos ? runtimeDir : "";
    auto arguments = std::to_string(ntohs(address.sin_port)) + " " + nonce + " \"" + dir + "\"";
    if (!start(script, arguments)) return Result::Failed;

    auto deadline = Clock::now() + START_WAIT;
    while (readable(listener.get(), deadline, cancelled)) {
        sockaddr_in peer {};
        int peerSize = sizeof(peer);
        Socket client(accept(listener.get(), reinterpret_cast<sockaddr*>(&peer), &peerSize));
        if (!client.valid()) continue;
        // The helper writes its proof before this byte.
        auto status = readByte(client.get(), std::min(deadline, Clock::now() + REPLY_WAIT), cancelled);
        if (!status || !proven(proof.path, ntohs(peer.sin_port))) continue;
        if (*status == 'K') {
            socket = client.release();
            return Result::Connected;
        }
        return *status == 'N' ? Result::NotRunning : Result::Failed;
    }
    return Result::Failed;
}
