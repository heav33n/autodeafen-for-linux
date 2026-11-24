#pragma once
#include "run_tracker.hpp"
#include <algorithm>
#include <optional>
#include <utility>

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
    LevelEdit edit {
        enabled,
        std::clamp(start.value_or(before.start), 0.f, 100.f),
        std::clamp(end.value_or(before.end), 0.f, 100.f),
    };
    if (edit.start > edit.end) std::swap(edit.start, edit.end);
    return edit;
}
