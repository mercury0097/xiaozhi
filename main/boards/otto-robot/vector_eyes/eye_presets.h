/**
 * @file eye_presets.h
 * @brief 18种表情预设参数
 * 
 * 移植自 esp32-eyes，参数已针对 240x240 屏幕缩放
 * 原始参数基于 128x64 屏幕，缩放系数约 1.875 (240/128)
 */

#pragma once

#include "eye_config.h"
#include "emotions.h"

namespace vector_eyes {

// 缩放系数：从 128x64 到 240x240
constexpr float SCALE = 2.0f;

// ============ 表情预设 ============

constexpr EyeConfig Preset_Normal = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(40 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(8 * SCALE),
    .radius_bottom = static_cast<int16_t>(8 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Happy = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(10 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(10 * SCALE),
    .radius_bottom = 0,
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Glee = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(8 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(8 * SCALE),
    .radius_bottom = 0,
    .inverse_radius_top = 0,
    .inverse_radius_bottom = static_cast<int16_t>(5 * SCALE),
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Sad = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(15 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = -0.5f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(1 * SCALE),
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Worried = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(25 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = -0.1f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(6 * SCALE),
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Focused = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(14 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0.2f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(3 * SCALE),
    .radius_bottom = static_cast<int16_t>(1 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Annoyed = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(12 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = 0,
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Surprised = {
    .offset_x = static_cast<int16_t>(-2 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(45 * SCALE),
    .width = static_cast<int16_t>(45 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(16 * SCALE),
    .radius_bottom = static_cast<int16_t>(16 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Skeptic = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(40 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(10 * SCALE),
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Frustrated = {
    .offset_x = static_cast<int16_t>(3 * SCALE),
    .offset_y = static_cast<int16_t>(-5 * SCALE),
    .height = static_cast<int16_t>(12 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = 0,
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Unimpressed = {
    .offset_x = static_cast<int16_t>(3 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(12 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(1 * SCALE),
    .radius_bottom = static_cast<int16_t>(10 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Sleepy = {
    .offset_x = 0,
    .offset_y = static_cast<int16_t>(-2 * SCALE),
    .height = static_cast<int16_t>(14 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = -0.5f,
    .slope_bottom = -0.5f,
    .radius_top = static_cast<int16_t>(3 * SCALE),
    .radius_bottom = static_cast<int16_t>(3 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Suspicious = {
    .offset_x = 0,
    .offset_y = 0,
    .height = static_cast<int16_t>(22 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(8 * SCALE),
    .radius_bottom = static_cast<int16_t>(3 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Squint = {
    .offset_x = static_cast<int16_t>(-10 * SCALE),
    .offset_y = static_cast<int16_t>(-3 * SCALE),
    .height = static_cast<int16_t>(35 * SCALE),
    .width = static_cast<int16_t>(35 * SCALE),
    .slope_top = 0,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(8 * SCALE),
    .radius_bottom = static_cast<int16_t>(8 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Angry = {
    .offset_x = static_cast<int16_t>(-3 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(20 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0.3f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(2 * SCALE),
    .radius_bottom = static_cast<int16_t>(12 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Furious = {
    .offset_x = static_cast<int16_t>(-2 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(30 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = 0.4f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(2 * SCALE),
    .radius_bottom = static_cast<int16_t>(8 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Scared = {
    .offset_x = static_cast<int16_t>(-3 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(40 * SCALE),
    .width = static_cast<int16_t>(40 * SCALE),
    .slope_top = -0.1f,
    .slope_bottom = 0,
    .radius_top = static_cast<int16_t>(12 * SCALE),
    .radius_bottom = static_cast<int16_t>(8 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

constexpr EyeConfig Preset_Awe = {
    .offset_x = static_cast<int16_t>(2 * SCALE),
    .offset_y = 0,
    .height = static_cast<int16_t>(35 * SCALE),
    .width = static_cast<int16_t>(45 * SCALE),
    .slope_top = -0.1f,
    .slope_bottom = 0.1f,
    .radius_top = static_cast<int16_t>(12 * SCALE),
    .radius_bottom = static_cast<int16_t>(12 * SCALE),
    .inverse_radius_top = 0,
    .inverse_radius_bottom = 0,
    .inverse_offset_top = 0,
    .inverse_offset_bottom = 0
};

/**
 * @brief 根据表情枚举获取预设
 */
inline const EyeConfig& GetPreset(Emotion emotion) {
    switch (emotion) {
        case Emotion::Normal:      return Preset_Normal;
        case Emotion::Happy:       return Preset_Happy;
        case Emotion::Glee:        return Preset_Glee;
        case Emotion::Sad:         return Preset_Sad;
        case Emotion::Worried:     return Preset_Worried;
        case Emotion::Focused:     return Preset_Focused;
        case Emotion::Annoyed:     return Preset_Annoyed;
        case Emotion::Surprised:   return Preset_Surprised;
        case Emotion::Skeptic:     return Preset_Skeptic;
        case Emotion::Frustrated:  return Preset_Frustrated;
        case Emotion::Unimpressed: return Preset_Unimpressed;
        case Emotion::Sleepy:      return Preset_Sleepy;
        case Emotion::Suspicious:  return Preset_Suspicious;
        case Emotion::Squint:      return Preset_Squint;
        case Emotion::Angry:       return Preset_Angry;
        case Emotion::Furious:     return Preset_Furious;
        case Emotion::Scared:      return Preset_Scared;
        case Emotion::Awe:         return Preset_Awe;
        default:                   return Preset_Normal;
    }
}

} // namespace vector_eyes
