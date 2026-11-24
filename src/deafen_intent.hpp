#pragma once
#include <optional>

// Remembers what the game wants and whether we are the one who deafened, so a
// player who deafened themself is never undeafened by us.
class DeafenIntent {
    bool m_wanted = false;
    bool m_ours = false;
    bool m_leaveAlone = false;

public:
    void want(bool deaf) {
        m_wanted = deaf;
        if (!deaf) m_leaveAlone = false;
    }

    bool needsCheck() const {
        return m_wanted ? !m_ours && !m_leaveAlone : m_ours;
    }

    // Given Discord's current state, returns the value to send, if any.
    std::optional<bool> next(bool discordDeaf) {
        if (!this->needsCheck()) return {};
        if (m_wanted) {
            if (!discordDeaf) return true;
            m_leaveAlone = true;
            return {};
        }
        if (discordDeaf) return false;
        m_ours = false;
        return {};
    }

    // Discord confirmed the value we sent.
    void applied(bool deaf) {
        m_ours = deaf;
    }

    void linkLost() {
        m_ours = false;
    }
};
