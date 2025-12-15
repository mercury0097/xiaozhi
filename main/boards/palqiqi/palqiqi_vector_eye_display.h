/**
 * @file otto_vector_eye_display.h
 * @brief Otto机器人矢量眼睛显示类
 * 
 * 使用矢量绘制的眼睛替代 GIF 表情
 */

#pragma once

#include <lvgl.h>
#include <string.h>

#include "display/lcd_display.h"
#include "vector_eyes/vector_face.h"
#include "vector_eyes/emotions.h"

/**
 * @brief Otto机器人矢量眼睛显示类
 * 继承SpiLcdDisplay，使用矢量绘制眼睛
 */
class PalqiqiVectorEyeDisplay : public SpiLcdDisplay {
public:
    /**
     * @brief 构造函数
     */
    PalqiqiVectorEyeDisplay(esp_lcd_panel_io_handle_t panel_io, 
                         esp_lcd_panel_handle_t panel, 
                         int width, int height, 
                         int offset_x, int offset_y, 
                         bool mirror_x, bool mirror_y, bool swap_xy);

    virtual ~PalqiqiVectorEyeDisplay();

    // 重写表情设置方法
    virtual void SetEmotion(const char* emotion) override;

    // 重写聊天消息设置方法
    virtual void SetChatMessage(const char* role, const char* content) override;

    // 重写主题设置方法 - 矢量眼睛使用固定风格
    virtual void SetTheme(Theme* theme) override;

    /**
     * @brief 手动触发眨眼
     */
    void Blink();

    /**
     * @brief 看向指定方向
     */
    void LookAt(float x, float y);

    /**
     * @brief 设置眼睛颜色
     */
    void SetEyeColor(uint32_t color_hex);

    /**
     * @brief 设置目标帧率
     * @param fps 目标帧率，范围 10-30 FPS
     * @return true 设置成功，false 参数超出范围
     * 
     * 验证帧率范围并更新 LVGL timer 周期
     * Requirements: 4.1
     */
    bool SetTargetFrameRate(uint8_t fps);

    /**
     * @brief 获取当前目标帧率
     * @return 当前设置的目标帧率
     */
    uint8_t GetTargetFrameRate() const { return target_fps_; }

    /**
     * @brief 获取实际帧率
     * @return 最近计算的实际帧率 (FPS)
     * 
     * 返回最近一秒内测量的实际帧率
     * Requirements: 4.1
     */
    float GetActualFrameRate() const { return actual_fps_; }

private:
    void SetupCanvas();
    void StartUpdateTimer();
    void StopUpdateTimer();
    
    static void UpdateTimerCallback(lv_timer_t* timer);
    void OnUpdate();

    // 表情名称到枚举的映射
    vector_eyes::Emotion MapEmotionName(const char* name);

    lv_obj_t* canvas_ = nullptr;
    lv_color_t* canvas_buf_ = nullptr;
    vector_eyes::VectorFace* face_ = nullptr;
    lv_timer_t* update_timer_ = nullptr;

    lv_obj_t* chat_message_label_ = nullptr;

    // 随机表情变化
    uint32_t last_emotion_change_ = 0;
    uint32_t next_emotion_interval_ = 0;
    vector_eyes::Emotion current_emotion_ = vector_eyes::Emotion::Normal;
    bool idle_mode_ = true;  // 空闲模式下才随机变化
    
    // 表情演示模式
    bool demo_mode_ = false;   // 禁用演示模式
    int demo_emotion_index_ = 0;
    uint32_t demo_start_time_ = 0;
    
    // 性能统计变量 (Requirements 3.3, 4.4)
    uint32_t frame_count_ = 0;           // 帧计数器
    uint32_t last_fps_update_ = 0;       // 上次FPS更新时间戳
    float actual_fps_ = 0.0f;            // 实际测量的帧率
    uint32_t last_render_time_ = 0;      // 上一帧渲染时间(ms)
    
    // 帧率配置 (Requirements 4.1)
    uint8_t target_fps_ = 20;            // 目标帧率，默认20 FPS
    static constexpr uint8_t kMinFps = 10;   // 最小帧率
    static constexpr uint8_t kMaxFps = 30;   // 最大帧率
    
    void CheckRandomEmotion();
    void CheckDemoMode();
    void ScheduleNextEmotionChange();

    // 表情名称映射表
    struct EmotionNameMap {
        const char* name;
        vector_eyes::Emotion emotion;
    };
    static const EmotionNameMap emotion_name_maps_[];
};
