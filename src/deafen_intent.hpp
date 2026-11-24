#pragma once
#include <optional>

// Remembers what the game wants and whether we are the one who deafened, so a
// player who deafened themself is never undeafened by us.
class DeafenIntent {
    bool m_wanted = false;
    // Set as soon as we ask, because the reply can get lost while the deafen
    // still happens.
    bool m_ours = false;
    bool m_confirmed = false;
    bool m_leaveAlone = false;

public:
    void want(bool deaf) {
        m_wanted = deaf;
        if (!deaf) m_leaveAlone = false;
    }

    bool needsCheck() const {
        return m_wanted ? !m_confirmed && !m_leaveAlone : m_ours;
    }

    // Given Discord's current state, returns the value to send, if any.
    std::optional<bool> next(bool discordDeaf) {
        if (!this->needsCheck()) return {};
        if (m_wanted) {
            if (!discordDeaf) {
                m_ours = true;
                return true;
            }
            if (m_ours) m_confirmed = true;
            else m_leaveAlone = true;
            return {};
        }
        if (discordDeaf) return false;
        m_ours = m_confirmed = false;
        return {};
    }

    // Discord confirmed the value we sent.
    void applied(bool deaf) {
        m_ours = m_confirmed = deaf;
    }

    // Discord said no. A refused deafen never happened; a refused undeafen is
    // still owed.
    void refused(bool deaf) {
        if (deaf) m_ours = m_confirmed = false;
    }

    bool owesUndeafen() const {
        return m_ours && !m_wanted;
    }

    // We are disconnecting, but the run may go on. Unlike want(false), this
    // remembers a player who undeafened themself.
    void release() {
        m_wanted = false;
    }

    // Discord's state changed outside our requests. If the player undeafened
    // themself, it is theirs for the rest of the run.
    void observed(bool deaf) {
        if (deaf || !m_wanted || !m_confirmed) return;
        m_ours = m_confirmed = false;
        m_leaveAlone = true;
    }

    void linkLost() {
        m_ours = m_confirmed = false;
    }
};
