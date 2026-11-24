#include "discord_socket.hpp"
#include "socket_paths.hpp"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <cstring>

namespace {
    // Not in every SDK, and the layout is fixed by the kernel.
    struct UnixAddress {
        unsigned short family;
        char path[108];
    };
    static_assert(sizeof(UnixAddress) == 110);
}

DiscordSocket::~DiscordSocket() {
    this->close();
}

bool DiscordSocket::isOpen() const {
    return m_socket != INVALID_SOCKET;
}

void DiscordSocket::close() {
    if (this->isOpen()) closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    m_reader = {};
}

DiscordSocket::Result DiscordSocket::connect(std::string const& runtimeDir, Helper const& helper) {
    this->close();
    static bool const s_started = [] {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    if (!s_started) return Result::Unsupported;

    if (!m_usesHelper) {
        auto probe = socket(AF_UNIX, SOCK_STREAM, 0);
        if (probe == INVALID_SOCKET) m_usesHelper = true;
        else closesocket(probe);
    }
    if (m_usesHelper) {
        auto script = helper.prepare();
        if (!script) return Result::Unsupported;
        switch (helper_link::connect(*script, runtimeDir, helper.cancelled, m_socket)) {
            case helper_link::Result::Connected: return Result::Connected;
            case helper_link::Result::NotRunning: return Result::NotRunning;
            case helper_link::Result::Failed: return Result::Unsupported;
        }
    }

    for (auto const& path : socketPaths(runtimeDir)) {
        auto sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) return Result::NotRunning;

        UnixAddress address { AF_UNIX, {} };
        std::memcpy(address.path, path.c_str(), path.size() + 1);
        if (::connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
            m_socket = sock;
            return Result::Connected;
        }
        closesocket(sock);
    }
    return Result::NotRunning;
}

bool DiscordSocket::send(uint32_t op, std::string_view json) {
    auto bytes = encodeFrame(op, json);
    std::string_view rest = bytes;
    while (!rest.empty()) {
        if (!this->isOpen()) return false;
        auto sent = ::send(m_socket, rest.data(), static_cast<int>(rest.size()), 0);
        if (sent <= 0) {
            this->close();
            return false;
        }
        rest.remove_prefix(sent);
    }
    return true;
}

std::optional<std::pair<uint32_t, std::string>> DiscordSocket::receive(std::chrono::milliseconds wait) {
    auto deadline = Clock::now() + wait;
    while (this->isOpen()) {
        if (auto message = m_reader.pop()) return message;
        if (m_reader.broken()) break;

        auto left = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now()).count();
        if (left < 0) left = 0;
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(m_socket, &readable);
        timeval timeout { static_cast<long>(left / 1000000), static_cast<long>(left % 1000000) };
        auto ready = select(0, &readable, nullptr, nullptr, &timeout);
        if (ready == 0) return {};
        if (ready < 0) break;

        char buffer[4096];
        auto got = recv(m_socket, buffer, sizeof(buffer), 0);
        if (got <= 0) break;
        m_reader.feed({ buffer, static_cast<size_t>(got) });
    }
    this->close();
    return {};
}
