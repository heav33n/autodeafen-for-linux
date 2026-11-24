#pragma once
#include "range.hpp"
#include <Geode/Geode.hpp>

// A slider with two handles.
class RangeSlider : public cocos2d::CCLayer {
    Range m_range {};
    float m_width = 0.f;
    std::optional<RangeThumb> m_dragging;
    std::function<void(Range)> m_onChange;
    cocos2d::CCSprite* m_fill = nullptr;
    cocos2d::CCSprite* m_start = nullptr;
    cocos2d::CCSprite* m_end = nullptr;

    bool init(float width, std::function<void(Range)> onChange);
    void drag(cocos2d::CCTouch* touch);
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;
    void ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
    void ccTouchCancelled(cocos2d::CCTouch*, cocos2d::CCEvent*) override;

public:
    static RangeSlider* create(float width, std::function<void(Range)> onChange);
    // Does not call onChange.
    void setRange(Range range);
    Range getRange() const { return m_range; }
};
