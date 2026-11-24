#include "auth.hpp"
#include "discord_link.hpp"
#include "level_config.hpp"
#include "level_popup.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace {
    RunTracker s_tracker;
    RunConfig s_config {};
    std::string s_levelKey;

    void apply(std::optional<bool> deaf) {
        if (deaf) link::want(*deaf);
    }
}

void current_run::configChanged() {
    s_config = level_config::load(s_levelKey);
    if (!s_config.enabled) apply(s_tracker.restarted());
}

$on_mod(Loaded) {
    auth::load();
    link::start();
}

$on_game(Exiting) {
    link::stop();
}

class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        apply(s_tracker.restarted());
        s_levelKey = level_config::keyFor(level);
        s_config = level_config::load(s_levelKey);
        return PlayLayer::init(level, dontCreateObjects);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (m_hasCompletedLevel || m_player1->m_isDead) return;
        apply(s_tracker.frame(s_config, this->getCurrentPercent(), m_isPracticeMode, m_level->isPlatformer()));
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (m_player1->m_isDead) apply(s_tracker.ended());
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        apply(s_tracker.ended());
    }

    void resetLevel() {
        apply(s_tracker.restarted());
        PlayLayer::resetLevel();
    }

    void onQuit() {
        apply(s_tracker.ended());
        PlayLayer::onQuit();
    }

    // Covers every way out of a level, including Edit in the pause menu, which
    // skips onQuit.
    void onExit() {
        apply(s_tracker.ended());
        PlayLayer::onExit();
    }
};

class $modify(AutoDeafenPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_musicOffBtn_001.png");
        sprite->setScale(.7f);
        auto button = CCMenuItemExt::createSpriteExtra(sprite, [](auto) {
            LevelPopup::create(s_levelKey)->show();
        });
        button->setID("deafen-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
    }
};
