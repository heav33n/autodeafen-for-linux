#pragma once
#include <algorithm>
#include <cmath>

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
    if (thumb == RangeThumb::Start) range.start = std::min(value, range.end);
    else range.end = std::max(value, range.start);
    return range;
}
