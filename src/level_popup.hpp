#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

// Opened from the pause menu: this level's settings and the Discord status.
class LevelPopup : public geode::Popup {
    std::string m_key;
    bool m_enabled = false;
    std::string m_percentText;
    geode::TextInput* m_percent = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;

    bool init(std::string levelKey);
    void refreshStatus(float);
    void save();

public:
    static LevelPopup* create(std::string levelKey);
    void onClose(cocos2d::CCObject* sender) override;
};
