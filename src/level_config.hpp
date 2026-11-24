#pragma once
#include "level_edit.hpp"
#include "run_tracker.hpp"
#include <Geode/Geode.hpp>

namespace level_config {
    std::string keyFor(GJGameLevel* level);
    RunConfig load(std::string const& key);
    void save(std::string const& key, LevelEdit const& edit);
}

// The run being played right now.
namespace current_run {
    // Re-reads the config, undeafening if it no longer covers where we are.
    void configChanged();
}
