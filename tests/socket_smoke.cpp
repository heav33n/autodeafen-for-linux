// Manual check, run under Proton: connects and says hello. Changes nothing.
#include "../src/discord_socket.hpp"
#include "../src/socket_paths.hpp"
#include <cstdio>
#include <cstdlib>
int main(int argc, char** argv) {
    if (argc < 4) return 2; // client ID, folder with the helper files, folder to install into
    auto dir = runtimeDir(std::getenv("WINE_HOST_XDG_RUNTIME_DIR"), std::getenv("XDG_RUNTIME_DIR"));
    std::printf("runtime dir: %s\n", dir.c_str());
    DiscordSocket s;
    auto result = s.connect(dir, {
        [&] { return helper_link::install(argv[2], argv[3]); }, [] { return false; }
    });
    std::printf("through helper: %d\n", s.usesHelper());
    std::printf("connect: %d\n", static_cast<int>(result));
    if (result != DiscordSocket::Result::Connected) return 1;
    s.send(0, std::string("{\"v\":1,\"client_id\":\"") + argv[1] + "\"}");
    auto reply = s.receive(std::chrono::seconds(5));
    std::printf("reply: op %u, ready %d\n", reply ? reply->first : 99, reply && reply->second.find("READY") != std::string::npos);
    auto quiet = s.receive(std::chrono::milliseconds(300));
    std::printf("idle: message %d, still open %d\n", quiet.has_value(), s.isOpen());
    return reply ? 0 : 1;
}
