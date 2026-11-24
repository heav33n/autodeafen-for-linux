#pragma once
#include <string>
#include <string_view>
#include <vector>

// Every place Discord may put its socket, most likely first.
inline std::vector<std::string> socketPaths(std::string_view runtimeDir) {
    constexpr size_t MAX_PATH_LENGTH = 107; // sockaddr_un holds 108 with the terminator
    std::vector<std::string> paths;
    if (runtimeDir.empty()) return paths;
    for (std::string_view sub : { "/", "/app/com.discordapp.Discord/" }) {
        for (int i = 0; i < 10; ++i) {
            auto path = std::string(runtimeDir).append(sub) + "discord-ipc-" + std::to_string(i);
            if (path.size() <= MAX_PATH_LENGTH) paths.push_back(std::move(path));
        }
    }
    return paths;
}
