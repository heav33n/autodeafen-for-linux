#include "range_slider.hpp"

using namespace geode::prelude;

namespace {
    constexpr float HEIGHT = 30.f;
}

RangeSlider* RangeSlider::create(float width, std::function<void(Range)> onChange) {
    auto ret = new RangeSlider();
    if (ret->init(width, std::move(onChange))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RangeSlider::init(float width, std::function<void(Range)> onChange) {
    if (!CCLayer::init()) return false;
    m_width = width;
    m_onChange = std::move(onChange);
    this->ignoreAnchorPointForPosition(false);
    this->setAnchorPoint({ .5f, .5f });
    this->setContentSize({ width, HEIGHT });

    m_fill = CCSprite::create("sliderBar.png");
    m_fill->setAnchorPoint({ 0.f, .5f });
    this->addChild(m_fill);

    auto groove = CCSprite::create("slidergroove.png");
    groove->setScaleX((width + 10.f) / groove->getContentWidth());
    groove->setPosition({ width / 2.f, HEIGHT / 2.f });
    this->addChild(groove);

    m_start = CCSprite::create("sliderthumb.png");
    m_end = CCSprite::create("sliderthumb.png");
    this->addChild(m_start);
    this->addChild(m_end);

    this->setRange({ 0.f, 100.f });
    this->setTouchEnabled(true);
    return true;
}

void RangeSlider::setRange(Range range) {
    m_range = range;
    auto x = [this](float value) { return value / 100.f * m_width; };
    m_start->setPosition({ x(range.start), HEIGHT / 2.f });
    m_end->setPosition({ x(range.end), HEIGHT / 2.f });
    m_fill->setPosition({ x(range.start), HEIGHT / 2.f });
    m_fill->setScaleX((x(range.end) - x(range.start)) / m_fill->getContentWidth());
}

void RangeSlider::drag(CCTouch* touch) {
    auto value = rangeValue(this->convertTouchToNodeSpace(touch).x, m_width);
    if (!m_dragging) {
        m_dragging = pickThumb(value, m_range.start, m_range.end);
        auto thumb = *m_dragging == RangeThumb::Start ? m_start : m_end;
        thumb->setTexture(CCTextureCache::get()->addImage("sliderthumbsel.png", false));
        this->reorderChild(thumb, 1);
        this->reorderChild(thumb == m_start ? m_end : m_start, 0);
    }
    auto moved = moveThumb(m_range, *m_dragging, value);
    if (moved == m_range) return;
    this->setRange(moved);
    m_onChange(moved);
}

void RangeSlider::registerWithTouchDispatcher() {
    CCTouchDispatcher::get()->addTargetedDelegate(this, -502, true);
}

bool RangeSlider::ccTouchBegan(CCTouch* touch, CCEvent*) {
    auto at = this->convertTouchToNodeSpace(touch);
    if (at.x < -15.f || at.x > m_width + 15.f || at.y < 0.f || at.y > HEIGHT) return false;
    this->drag(touch);
    return true;
}

void RangeSlider::ccTouchMoved(CCTouch* touch, CCEvent*) {
    this->drag(touch);
}

void RangeSlider::ccTouchEnded(CCTouch*, CCEvent*) {
    m_dragging.reset();
    for (auto thumb : { m_start, m_end }) {
        thumb->setTexture(CCTextureCache::get()->addImage("sliderthumb.png", false));
    }
}

void RangeSlider::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    this->ccTouchEnded(touch, event);
}
