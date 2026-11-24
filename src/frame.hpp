#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

// Discord's local messages: opcode, length (both little endian), then JSON.
namespace frame_detail {
    inline void put32(std::string& out, uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            out.push_back(static_cast<char>((value >> shift) & 0xff));
        }
    }

    inline uint32_t get32(std::string_view in) {
        uint32_t value = 0;
        for (int i = 3; i >= 0; --i) {
            value = (value << 8) | static_cast<unsigned char>(in[i]);
        }
        return value;
    }
}

inline std::string encodeFrame(uint32_t op, std::string_view json) {
    std::string out;
    out.reserve(8 + json.size());
    frame_detail::put32(out, op);
    frame_detail::put32(out, static_cast<uint32_t>(json.size()));
    out.append(json);
    return out;
}

class FrameReader {
    static constexpr uint32_t MAX_BODY = 1024 * 1024;
    std::string m_buffer;
    bool m_broken = false;

public:
    void feed(std::string_view bytes) {
        m_buffer.append(bytes);
    }

    std::optional<std::pair<uint32_t, std::string>> pop() {
        if (m_broken || m_buffer.size() < 8) return {};
        std::string_view view = m_buffer;
        auto size = frame_detail::get32(view.substr(4));
        if (size > MAX_BODY) {
            m_broken = true;
            return {};
        }
        if (view.size() < 8 + static_cast<size_t>(size)) return {};
        std::pair<uint32_t, std::string> result { frame_detail::get32(view), view.substr(8, size) };
        m_buffer.erase(0, 8 + static_cast<size_t>(size));
        return result;
    }

    bool broken() const {
        return m_broken;
    }
};
