#pragma once
#include <optional>

struct RunConfig {
    bool enabled;
    float percent;
    bool practice;
};

// Decides when a run should deafen or undeafen.
// true = deafen, false = undeafen, nullopt = nothing to do.
class RunTracker {
    bool m_deafened = false;
    bool m_over = false;

public:
    std::optional<bool> frame(RunConfig config, float percent, bool inPractice, bool platformer) {
        if (m_deafened || m_over || !config.enabled || platformer) return {};
        if (inPractice && !config.practice) return {};
        if (percent < config.percent || percent >= 100.f) return {};
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
};
