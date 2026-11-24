#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

struct RunConfig {
    bool enabled;
    // Deafened from start up to end, in percent. An end of 100 means until the
    // run is over.
    float start, end;
    bool practice;
};

// Settings come from files people can edit, so never trust the numbers.
inline RunConfig sanitized(RunConfig config) {
    config.start = std::isfinite(config.start) ? std::clamp(config.start, 0.f, 100.f) : 50.f;
    config.end = std::isfinite(config.end) ? std::clamp(config.end, 0.f, 100.f) : 100.f;
    if (config.start > config.end) std::swap(config.start, config.end);
    if (config.start == config.end) {
        config.end = std::min(config.start + 1.f, 100.f);
        config.start = std::min(config.start, config.end - 1.f);
    }
    return config;
}

// Decides when a run should deafen or undeafen.
// true = deafen, false = undeafen, nullopt = nothing to do.
class RunTracker {
    bool m_deafened = false;
    bool m_over = false;

    static bool inRange(RunConfig config, float percent) {
        return percent >= config.start && percent < config.end;
    }

public:
    std::optional<bool> frame(RunConfig config, float percent, bool inPractice, bool platformer) {
        if (m_over || platformer) return {};
        if (m_deafened) {
            if (inPractice && !config.practice) {
                m_deafened = false;
                return false;
            }
            if (config.end < 100.f && percent >= config.end) return this->ended();
            return {};
        }
        if (!config.enabled || (inPractice && !config.practice)) return {};
        if (!inRange(config, percent) || percent >= 100.f) return {};
        m_deafened = true;
        return true;
    }

    // Death, completion or quitting. Frames keep coming after these, so stay
    // quiet until the next attempt.
    std::optional<bool> ended() {
        m_over = true;
        if (!m_deafened) return {};
        m_deafened = false;
        return false;
    }

    std::optional<bool> restarted() {
        auto result = this->ended();
        m_over = false;
        return result;
    }

    // The player changed the settings mid-run.
    std::optional<bool> configChanged(RunConfig config, float percent) {
        m_over = false;
        if (!m_deafened || (config.enabled && inRange(config, percent))) return {};
        m_deafened = false;
        return false;
    }
};
