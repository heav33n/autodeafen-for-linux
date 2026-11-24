#pragma once
#include "range_slider.hpp"
#include "run_tracker.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

// Opened from the pause menu: this level's settings and the Discord status.
class LevelPopup : public geode::Popup {
    std::string m_key;
    bool m_enabled = false;
    RunConfig m_before {};
    // Only the values the player touched.
    std::optional<float> m_startEdit, m_endEdit;
    RangeSlider* m_slider = nullptr;
    geode::TextInput* m_startInput = nullptr;
    geode::TextInput* m_endInput = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;

    bool init(std::string levelKey);
    geode::TextInput* addInput(char const* label, float x, float value, std::optional<float>& edit);
    void refreshStatus(float);
    void save();

public:
    static LevelPopup* create(std::string levelKey);
    void onClose(cocos2d::CCObject* sender) override;
};
