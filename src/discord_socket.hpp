#pragma once
#include "frame.hpp"
#include "helper_link.hpp"
#include <chrono>
#include <functional>
#include <optional>
#include <string>

// Discord's local Unix socket, reached through Wine's Winsock.
class DiscordSocket {
    uintptr_t m_socket = ~uintptr_t(0);
    FrameReader m_reader;
    bool m_usesHelper = false;

public:
    enum class Result { Connected, NotRunning, Unsupported };
    using Clock = std::chrono::steady_clock;

    struct Helper {
        // Called on first need. Gives the script to start, if there is one.
        std::function<std::optional<std::filesystem::path>()> prepare;
        std::function<bool()> cancelled;
    };

    DiscordSocket() = default;
    DiscordSocket(DiscordSocket const&) = delete;
    DiscordSocket& operator=(DiscordSocket const&) = delete;
    ~DiscordSocket();

    Result connect(std::string const& runtimeDir, Helper const& helper);
    // Whether this Proton needs the helper. Known after connecting once.
    bool usesHelper() const { return m_usesHelper; }
    void close();
    bool isOpen() const;
    bool send(uint32_t op, std::string_view json);
    // Waits up to `wait` for one message. Closes the socket if the link is lost.
    std::optional<std::pair<uint32_t, std::string>> receive(std::chrono::milliseconds wait);
};
