#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

// Asks for the user's own Discord app once, then has Discord authorize it.
class SetupPopup : public geode::Popup {
    geode::TextInput* m_id = nullptr;
    geode::TextInput* m_secret = nullptr;

    bool init() override;
    void authorize();

public:
    static SetupPopup* create();
};
