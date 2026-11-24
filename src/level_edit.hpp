#pragma once
#include "run_tracker.hpp"
#include <algorithm>
#include <optional>

struct LevelEdit {
    bool enabled;
    float percent;
};

// What closing the popup should save. Nothing if the player changed nothing, so
// the level keeps following the defaults.
inline std::optional<LevelEdit> levelEdit(
    RunConfig before, bool enabled, bool percentTouched, std::optional<float> percent
) {
    bool newPercent = percentTouched && percent.has_value();
    if (enabled == before.enabled && !newPercent) return {};
    return LevelEdit { enabled, newPercent ? std::clamp(*percent, 0.f, 100.f) : before.percent };
}
