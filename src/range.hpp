#pragma once
#include <algorithm>
#include <cmath>
#include <string>

enum class RangeThumb { Start, End };

struct Range {
    float start, end;
    bool operator==(Range const&) const = default;
};

// Percent at a position along a slider, in whole percents.
inline float rangeValue(float x, float width) {
    return std::clamp(std::round(x / width * 100.f), 0.f, 100.f);
}

// The handle to grab. When both are in the same place, the one that can move
// toward the touch.
inline RangeThumb pickThumb(float value, float start, float end) {
    if (start == end) {
        if (start >= 100.f) return RangeThumb::Start;
        if (end <= 0.f) return RangeThumb::End;
        return value < start ? RangeThumb::Start : RangeThumb::End;
    }
    return std::abs(value - start) <= std::abs(value - end) ? RangeThumb::Start : RangeThumb::End;
}

inline Range moveThumb(Range range, RangeThumb thumb, float value) {
    // Always at least one percent apart, or it would never deafen.
    if (thumb == RangeThumb::Start) range.start = std::max(0.f, std::min(value, range.end - 1.f));
    else range.end = std::min(100.f, std::max(value, range.start + 1.f));
    return range;
}

// Two decimals at most, without trailing zeros.
inline std::string formatPercent(float value) {
    auto hundredths = std::lround(value * 100.f);
    auto text = std::to_string(hundredths / 100);
    auto rest = hundredths % 100;
    if (rest == 0) return text;
    text += '.';
    text += static_cast<char>('0' + rest / 10);
    if (rest % 10) text += static_cast<char>('0' + rest % 10);
    return text;
}
