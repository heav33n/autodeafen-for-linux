// Runs on Linux, next to the game. Proton versions that cannot open Discord's
// socket start this instead: it connects to the mod and to Discord, and passes
// bytes between them until either side closes.
#include "../src/socket_paths.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
    constexpr size_t NONCE_LENGTH = 32;

    bool sendAll(int fd, char const* data, size_t size) {
        while (size > 0) {
            auto sent = send(fd, data, size, MSG_NOSIGNAL);
            if (sent < 0 && errno == EINTR) continue;
            if (sent <= 0) return false;
            data += sent;
            size -= sent;
        }
        return true;
    }

    int connectToMod(int port) {
        auto fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) return -1;
        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) return fd;
        close(fd);
        return -1;
    }

    // Writes our end of the connection into the mod's own folder. Other users on
    // this PC cannot write there, so the mod knows this connection is ours.
    bool writeProof(int mod, std::string const& folder, std::string const& nonce) {
        sockaddr_in address {};
        socklen_t size = sizeof(address);
        if (getsockname(mod, reinterpret_cast<sockaddr*>(&address), &size) != 0) return false;
        auto text = std::to_string(ntohs(address.sin_port)) + "\n";
        auto path = folder + "/autodeafen-" + nonce;
        auto temporary = path + ".tmp";
        auto fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return false;
        bool written = write(fd, text.data(), text.size()) == static_cast<ssize_t>(text.size());
        close(fd);
        // Renaming makes it appear all at once.
        if (written && std::rename(temporary.c_str(), path.c_str()) == 0) return true;
        unlink(temporary.c_str());
        return false;
    }

    int connectToDiscord(std::string const& dir) {
        for (auto const& path : socketPaths(dir)) {
            auto fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
            if (fd < 0) return -1;
            sockaddr_un address {};
            address.sun_family = AF_UNIX;
            std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
            if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) return fd;
            close(fd);
        }
        return -1;
    }

    // One direction. False when that side is done.
    bool pass(int from, int to) {
        static char s_buffer[65536];
        auto got = recv(from, s_buffer, sizeof(s_buffer), 0);
        if (got < 0 && errno == EINTR) return true;
        return got > 0 && sendAll(to, s_buffer, got);
    }

    // Each direction gets its own process, so a slow reader on one side can never
    // block the other.
    void pump(int from, int to) {
        while (pass(from, to)) {}
        // Wakes the other direction too.
        shutdown(from, SHUT_RDWR);
        shutdown(to, SHUT_RDWR);
    }
}

// Arguments: the mod's folder, port, nonce, runtime folder (may be empty).
int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);
    if (argc != 5) return 2;
    std::string folder = argv[1], nonce = argv[3];
    if (nonce.size() != NONCE_LENGTH) return 2;
    if (nonce.find_first_not_of("0123456789abcdef") != std::string::npos) return 2;
    char* rest = nullptr;
    auto port = std::strtol(argv[2], &rest, 10);
    if (rest == argv[2] || *rest || port < 1 || port > 65535) return 2;

    auto mod = connectToMod(static_cast<int>(port));
    if (mod < 0) return 3;
    if (!writeProof(mod, folder, nonce)) return 4;

    auto dir = runtimeDir(argv[4], std::getenv("XDG_RUNTIME_DIR"));
    if (dir.empty()) dir = "/run/user/" + std::to_string(getuid());
    auto discord = connectToDiscord(dir);
    if (!sendAll(mod, discord < 0 ? "N" : "K", 1) || discord < 0) return 0;

    auto child = fork();
    if (child < 0) return 5;
    if (child == 0) {
        pump(mod, discord);
        _exit(0);
    }
    pump(discord, mod);
    return 0;
}
