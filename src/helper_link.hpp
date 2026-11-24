#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

// Reaches Discord through the Linux helper, for Proton versions that cannot
// open Discord's socket themselves.
namespace helper_link {
    enum class Result { Connected, NotRunning, Failed };

    // Copies the packaged files into `to` under names Wine makes executable and
    // returns the script to start, or nothing if that failed.
    std::optional<std::filesystem::path> install(std::filesystem::path const& from, std::filesystem::path const& to);

    // On Connected, `socket` is a connection that behaves like Discord's own.
    Result connect(
        std::filesystem::path const& script, std::string const& runtimeDir,
        std::function<bool()> const& cancelled, uintptr_t& socket
    );
}
