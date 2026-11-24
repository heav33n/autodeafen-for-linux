#include "level_config.hpp"
#include "level_key.hpp"

using namespace geode::prelude;

std::string level_config::keyFor(GJGameLevel* level) {
    auto kind = LevelKind::Online;
    if (level->m_levelType == GJLevelType::Main) kind = LevelKind::Main;
    else if (level->m_levelType == GJLevelType::Editor) kind = LevelKind::Created;
    else if (level->m_dailyID > 0) kind = LevelKind::Daily;
    else if (level->m_gauntletLevel) kind = LevelKind::Gauntlet;
    return levelKey(kind, level->m_levelID.value(), level->m_levelName, level->m_levelRev);
}

RunConfig level_config::load(std::string const& key) {
    auto mod = Mod::get();
    RunConfig config {
        .enabled = mod->getSettingValue<bool>("enabled"),
        .percent = static_cast<float>(mod->getSettingValue<double>("percent")),
        .practice = mod->getSettingValue<bool>("practice"),
    };
    if (key.empty() || !mod->hasSavedValue(key)) return config;
    auto saved = mod->getSavedValue<matjson::Value>(key);
    config.enabled = saved["enabled"].asBool().unwrapOr(bool(config.enabled));
    config.percent = static_cast<float>(saved["percent"].asDouble().unwrapOr(double(config.percent)));
    return config;
}

void level_config::save(std::string const& key, bool enabled, float percent) {
    Mod::get()->setSavedValue(key, matjson::makeObject({
        { "enabled", enabled }, { "percent", static_cast<double>(percent) }
    }));
}
