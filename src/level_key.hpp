#pragma once
#include <string>

enum class LevelKind { Main, Online, Daily, Gauntlet, Created };

// The save key for a level. GD hands created levels a new ID every session, so
// those go by name and revision.
inline std::string levelKey(LevelKind kind, int id, std::string const& name, int revision) {
    auto number = std::to_string(id);
    switch (kind) {
        case LevelKind::Main: return "level-main-" + number;
        case LevelKind::Daily: return "level-daily-" + number;
        case LevelKind::Gauntlet: return "level-gauntlet-" + number;
        case LevelKind::Created: return "level-created-" + std::to_string(revision) + "-" + name;
        case LevelKind::Online: break;
    }
    return "level-online-" + number;
}
