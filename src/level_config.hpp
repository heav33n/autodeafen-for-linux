#pragma once
#include "run_tracker.hpp"
#include <Geode/Geode.hpp>

namespace level_config {
    std::string keyFor(GJGameLevel* level);
    RunConfig load(std::string const& key);
    void save(std::string const& key, bool enabled, float percent);
}

// The run being played right now.
namespace current_run {
    // Re-reads the config, undeafening if the level was just turned off.
    void configChanged();
}
