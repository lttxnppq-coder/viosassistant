#pragma once

#include <cstdint>

namespace utils {

class MovingAverageFilter {
public:
    explicit MovingAverageFilter(uint8_t window_size = 10);
    void begin(uint8_t window_size = 10);
    void reset();
    float update(float value);
    float getValue() const;
    uint8_t getCount() const;
    bool isReady() const;

private:
    float* buffer_ = nullptr;
    uint8_t window_size_ = 0;
    uint8_t index_ = 0;
    uint8_t count_ = 0;
    float sum_ = 0.0f;
};

class ExponentialFilter {
public:
    explicit ExponentialFilter(float alpha = 0.2f);
    void begin(float alpha = 0.2f);
    void reset(float initial_value = 0.0f);
    float update(float value);
    float getValue() const;
    void setAlpha(float alpha);

private:
    float alpha_ = 0.2f;
    float value_ = 0.0f;
    bool initialized_ = false;
};

class MedianFilter {
public:
    static constexpr uint8_t kMaxWindowSize = 15;
    static constexpr uint8_t kDefaultWindowSize = 5;

    explicit MedianFilter(uint8_t window_size = kDefaultWindowSize);
    void begin(uint8_t window_size = kDefaultWindowSize);
    void reset();
    float update(float value);
    float getValue() const;
    bool isReady() const;
    uint8_t getWindowSize() const { return window_size_; }

private:
    float* buffer_ = nullptr;
    uint8_t window_size_ = 0;
    uint8_t index_ = 0;
    uint8_t count_ = 0;
};

} // namespace utils
