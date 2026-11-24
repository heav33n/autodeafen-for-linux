#pragma once
#include <cstdint>
#include <string>
#include <string_view>

// The mod and the helper each write one line into the mod's folder saying which
// connection is theirs. Other users on the PC cannot write there. The helper
// always connects from 127.0.0.1, and the rest of 127.x.x.x must not pass.
inline std::string proofText(uint16_t port) {
    return "127.0.0.1 " + std::to_string(port) + "\n";
}

// The address is in host byte order. The final newline shows it is complete.
inline bool proofMatches(std::string_view proof, uint32_t address, uint16_t port) {
    return address == 0x7f000001 && proof == proofText(port);
}
