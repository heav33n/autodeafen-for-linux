#pragma once
#include "frame.hpp"
#include <chrono>
#include <functional>
#include <optional>
#include <string>

// Discord's local Unix socket, reached through Wine's Winsock.
class DiscordSocket {
    uintptr_t m_socket = ~uintptr_t(0);
    FrameReader m_reader;

public:
    enum class Result { Connected, NotRunning, Unsupported };
    using Clock = std::chrono::steady_clock;

    DiscordSocket() = default;
    DiscordSocket(DiscordSocket const&) = delete;
    DiscordSocket& operator=(DiscordSocket const&) = delete;
    ~DiscordSocket();

    Result connect(std::string const& runtimeDir);
    void close();
    bool isOpen() const;
    bool send(uint32_t op, std::string_view json);
    // Waits up to `wait` for one message. Closes the socket if the link is lost.
    std::optional<std::pair<uint32_t, std::string>> receive(std::chrono::milliseconds wait);
};
