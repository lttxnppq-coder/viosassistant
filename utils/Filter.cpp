#include "Filter.h"
#include <stdlib.h>
#include <string.h>

namespace utils {

MovingAverageFilter::MovingAverageFilter(uint8_t window_size) {
    begin(window_size);
}

void MovingAverageFilter::begin(uint8_t window_size) {
    if (buffer_) {
        free(buffer_);
    }
    window_size_ = (window_size > 0) ? window_size : 1;
    buffer_ = (float*)malloc(window_size_ * sizeof(float));
    if (buffer_) {
        memset(buffer_, 0, window_size_ * sizeof(float));
    }
    index_ = 0;
    count_ = 0;
    sum_ = 0.0f;
}

void MovingAverageFilter::reset() {
    if (buffer_) {
        memset(buffer_, 0, window_size_ * sizeof(float));
    }
    index_ = 0;
    count_ = 0;
    sum_ = 0.0f;
}

float MovingAverageFilter::update(float value) {
    if (!buffer_) return value;

    sum_ -= buffer_[index_];
    buffer_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) % window_size_;
    if (count_ < window_size_) count_++;
    return sum_ / count_;
}

float MovingAverageFilter::getValue() const {
    if (count_ == 0) return 0.0f;
    return sum_ / count_;
}

uint8_t MovingAverageFilter::getCount() const {
    return count_;
}

bool MovingAverageFilter::isReady() const {
    return count_ == window_size_;
}

ExponentialFilter::ExponentialFilter(float alpha) {
    begin(alpha);
}

void ExponentialFilter::begin(float alpha) {
    alpha_ = (alpha > 0.0f && alpha <= 1.0f) ? alpha : 0.2f;
    initialized_ = false;
}

void ExponentialFilter::reset(float initial_value) {
    value_ = initial_value;
    initialized_ = true;
}

float ExponentialFilter::update(float value) {
    if (!initialized_) {
        value_ = value;
        initialized_ = true;
        return value_;
    }
    value_ = alpha_ * value + (1.0f - alpha_) * value_;
    return value_;
}

float ExponentialFilter::getValue() const {
    return value_;
}

void ExponentialFilter::setAlpha(float alpha) {
    alpha_ = (alpha > 0.0f && alpha <= 1.0f) ? alpha : 0.2f;
}

MedianFilter::MedianFilter(uint8_t window_size) {
    begin(window_size);
}

void MedianFilter::begin(uint8_t window_size) {
    if (buffer_) {
        free(buffer_);
    }
    // Guard: clamp to kMaxWindowSize (production bug fix: sorted[15] overflow if window >15)
    // API allows any uint8_t, but internal sorted[] is fixed 15 -> must not exceed.
    if (window_size == 0) window_size = 1;
    if (window_size > kMaxWindowSize) window_size = kMaxWindowSize;
    // Ensure odd window for median (even -> odd+1, but re-clamp if overflow)
    if (window_size % 2 == 0) {
        if (window_size < kMaxWindowSize) window_size++;
        else window_size--; // 15 is odd, but if max even guard, step down to 15
    }
    window_size_ = window_size;
    buffer_ = (float*)malloc(window_size_ * sizeof(float));
    if (buffer_) {
        memset(buffer_, 0, window_size_ * sizeof(float));
    }
    index_ = 0;
    count_ = 0;
}

void MedianFilter::reset() {
    if (buffer_) {
        memset(buffer_, 0, window_size_ * sizeof(float));
    }
    index_ = 0;
    count_ = 0;
}

static int compareFloat(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

float MedianFilter::update(float value) {
    if (!buffer_) return value;

    buffer_[index_] = value;
    index_ = (index_ + 1) % window_size_;
    if (count_ < window_size_) count_++;

    float sorted[kMaxWindowSize];
    for (uint8_t i = 0; i < count_; i++) {
        uint8_t idx = (index_ + window_size_ - count_ + i) % window_size_;
        sorted[i] = buffer_[idx];
    }
    qsort(sorted, count_, sizeof(float), compareFloat);
    return sorted[count_ / 2];
}

float MedianFilter::getValue() const {
    if (count_ == 0) return 0.0f;
    float sorted[kMaxWindowSize];
    for (uint8_t i = 0; i < count_; i++) {
        uint8_t idx = (index_ + window_size_ - count_ + i) % window_size_;
        sorted[i] = buffer_[idx];
    }
    qsort(sorted, count_, sizeof(float), compareFloat);
    return sorted[count_ / 2];
}

bool MedianFilter::isReady() const {
    return count_ == window_size_;
}

} // namespace utils
