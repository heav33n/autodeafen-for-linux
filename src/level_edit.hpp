#pragma once
#include "run_tracker.hpp"
#include <optional>

struct LevelEdit {
    bool enabled;
    float start, end;
};

// What closing the popup should save. Nothing if the player changed nothing, so
// the level keeps following the defaults. Untouched or unreadable values are
// nullopt.
inline std::optional<LevelEdit> levelEdit(
    RunConfig before, bool enabled, std::optional<float> start, std::optional<float> end
) {
    if (enabled == before.enabled && !start && !end) return {};
    auto fixed = sanitized({
        .enabled = enabled,
        .start = start.value_or(before.start),
        .end = end.value_or(before.end),
        .practice = before.practice,
    });
    return LevelEdit { enabled, fixed.start, fixed.end };
}
