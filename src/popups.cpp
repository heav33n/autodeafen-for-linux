#include "auth.hpp"
#include "discord_link.hpp"
#include "level_edit.hpp"
#include "level_config.hpp"
#include "level_popup.hpp"
#include "setup_popup.hpp"
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace {
    std::pair<char const*, ccColor3B> describe(LinkStatus status) {
        switch (status) {
            case LinkStatus::Connected: return { "Discord: Connected", { 80, 220, 110 } };
            case LinkStatus::WaitingForApproval: return { "Press Authorize in Discord", { 255, 210, 80 } };
            case LinkStatus::BadApp: return { "Discord does not know that client ID", { 255, 100, 100 } };
            case LinkStatus::Refused: return { "Discord refused, trying again", { 255, 210, 80 } };
            case LinkStatus::NeedSetup: return { "Discord: Setup needed", { 255, 210, 80 } };
            case LinkStatus::Unsupported: return { "This Proton version is not supported", { 255, 100, 100 } };
            case LinkStatus::Disconnected: break;
        }
        return { "Discord: Disconnected", { 255, 100, 100 } };
    }

    CCMenuItemSpriteExtra* makeButton(char const* text, std::function<void()> onPress) {
        auto sprite = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_01.png", .8f);
        sprite->setScale(.7f);
        return CCMenuItemExt::createSpriteExtra(sprite, [onPress](auto) { onPress(); });
    }

    void addLabel(CCNode* parent, char const* text, float scale, CCPoint offset, char const* font = "bigFont.fnt") {
        auto label = CCLabelBMFont::create(text, font);
        label->setScale(scale);
        parent->addChildAtPosition(label, Anchor::Center, offset);
    }
}

LevelPopup* LevelPopup::create(std::string levelKey) {
    auto ret = new LevelPopup();
    if (ret->init(std::move(levelKey))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool LevelPopup::init(std::string levelKey) {
    if (!Popup::init(280.f, 230.f)) return false;
    this->setID("level-popup"_spr);
    this->setTitle("AutoDeafen");
    m_key = std::move(levelKey);
    m_before = level_config::load(m_key);
    m_enabled = m_before.enabled;

    addLabel(m_mainLayer, "Enabled", .5f, { -40.f, 65.f });
    auto toggle = CCMenuItemExt::createTogglerWithStandardSprites(.7f, [this](CCMenuItemToggler* sender) {
        m_enabled = !sender->isToggled();
    });
    toggle->toggle(m_enabled);
    m_buttonMenu->addChildAtPosition(toggle, Anchor::Center, { 50.f, 65.f });

    m_startInput = this->addInput("Deafen %", -60.f, m_before.start, m_startEdit);
    m_endInput = this->addInput("Undeafen %", 60.f, m_before.end, m_endEdit);

    m_slider = RangeSlider::create(200.f, [this](Range range) {
        if (range.start != m_startEdit.value_or(m_before.start)) {
            m_startEdit = range.start;
            m_startInput->setString(formatPercent(range.start));
        }
        if (range.end != m_endEdit.value_or(m_before.end)) {
            m_endEdit = range.end;
            m_endInput->setString(formatPercent(range.end));
        }
    });
    if (!m_slider) return false;
    m_slider->setRange({ m_before.start, m_before.end });
    m_mainLayer->addChildAtPosition(m_slider, Anchor::Center, { 0.f, -30.f });

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(.7f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Center, { 0.f, -58.f });
    this->refreshStatus(0.f);
    this->schedule(schedule_selector(LevelPopup::refreshStatus), .5f);

    m_buttonMenu->addChildAtPosition(
        makeButton("Discord Setup", [] { if (auto popup = SetupPopup::create()) popup->show(); }), Anchor::Center, { 0.f, -88.f }
    );
    handleTouchPriority(this);
    return true;
}

TextInput* LevelPopup::addInput(char const* label, float x, float value, std::optional<float>& edit) {
    addLabel(m_mainLayer, label, .4f, { x, 32.f });
    auto input = TextInput::create(80.f, "0-100");
    input->setCommonFilter(CommonFilter::Float);
    input->setMaxCharCount(6);
    input->setString(formatPercent(value));
    input->setCallback([this, &edit](std::string const& text) {
        auto typed = numFromString<float>(text).ok();
        // Unreadable, such as an empty box, means keep what was saved.
        if (typed) edit = std::clamp(*typed, 0.f, 100.f);
        else edit.reset();
        auto start = m_startEdit.value_or(m_before.start), end = m_endEdit.value_or(m_before.end);
        m_slider->setRange({ std::min(start, end), std::max(start, end) });
    });
    m_mainLayer->addChildAtPosition(input, Anchor::Center, { x, 8.f });
    return input;
}

void LevelPopup::refreshStatus(float) {
    auto [text, color] = describe(link::status());
    m_status->setString(text);
    m_status->setColor(color);
}

void LevelPopup::save() {
    auto edit = levelEdit(m_before, m_enabled, m_startEdit, m_endEdit);
    if (!edit) return;
    level_config::save(m_key, *edit);
    current_run::configChanged();
}

void LevelPopup::onClose(CCObject* sender) {
    this->save();
    Popup::onClose(sender);
}

SetupPopup* SetupPopup::create() {
    auto ret = new SetupPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SetupPopup::init() {
    if (!Popup::init(340.f, 240.f)) return false;
    this->setID("setup-popup"_spr);
    this->setTitle("Discord Setup");

    addLabel(m_mainLayer, "1. Create an app and add this OAuth2 redirect:", .6f, { 0.f, 75.f }, "chatFont.fnt");
    addLabel(m_mainLayer, "http://localhost", .6f, { 0.f, 60.f }, "goldFont.fnt");
    m_buttonMenu->addChildAtPosition(makeButton("Open Discord Portal", [] {
        web::openLinkInBrowser("https://discord.com/developers/applications");
    }), Anchor::Center, { 0.f, 35.f });
    addLabel(m_mainLayer, "2. Paste its client ID and secret:", .6f, { 0.f, 10.f }, "chatFont.fnt");

    m_id = TextInput::create(260.f, "Client ID");
    m_id->setFilter("0123456789");
    m_id->setMaxCharCount(24);
    m_id->setString(auth::current().appId);
    m_mainLayer->addChildAtPosition(m_id, Anchor::Center, { 0.f, -15.f });

    m_secret = TextInput::create(260.f, "Client Secret");
    m_secret->setCommonFilter(CommonFilter::Any);
    m_secret->setMaxCharCount(64);
    m_secret->setPasswordMode(true);
    m_mainLayer->addChildAtPosition(m_secret, Anchor::Center, { 0.f, -50.f });

    m_buttonMenu->addChildAtPosition(
        makeButton("Authorize", [this] { this->authorize(); }), Anchor::Center, { 0.f, -90.f }
    );
    return true;
}

void SetupPopup::authorize() {
    auto trim = [](std::string text) { return string::trim(text); };
    auto id = trim(m_id->getString());
    auto secret = trim(m_secret->getString());
    auto saved = auth::current();
    // An empty secret keeps the saved one, so logging in again is one click.
    if (secret.empty() && id == saved.appId) secret = saved.secret;
    if (id.empty() || secret.empty()) {
        Notification::create("Enter the client ID and secret", NotificationIcon::Error)->show();
        return;
    }
    if (id != saved.appId || secret != saved.secret) auth::setApp(id, secret);
    link::authorize();
    this->onClose(nullptr);
}
