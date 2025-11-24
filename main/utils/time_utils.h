#pragma once

#include <cstdint>
#include <esp_timer.h>
#include <sys/time.h>

namespace xiaozhi {
namespace time_utils {

/**
 * @brief 获取当前 epoch 时间（毫秒）
 */
inline int64_t GetEpochTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

/**
 * @brief 获取当前小时（0-23）
 */
inline int GetCurrentHour() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo.tm_hour;
}

/**
 * @brief 获取当天午夜的时间戳（毫秒）
 */
inline int64_t GetMidnightTimeMs() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    time_t midnight = mktime(&timeinfo);
    return (int64_t)midnight * 1000LL;
}

/**
 * @brief 获取高精度计时器时间（微秒，用于性能测试）
 */
inline int64_t GetTimerUs() {
    return esp_timer_get_time();
}

} // namespace time_utils
} // namespace xiaozhi























