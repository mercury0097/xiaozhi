/**
 * @file otto_vector_eye_display.cc
 * @brief Otto机器人矢量眼睛显示类实现
 */

#include "palqiqi_vector_eye_display.h"
#include "display/lvgl_display/lvgl_theme.h"
#include "application.h"

#include <cstring>
#include <esp_log.h>

#define TAG "PalqiqiVectorEyeDisplay"

// 表情名称映射表 - 将现有表情名映射到矢量表情
const PalqiqiVectorEyeDisplay::EmotionNameMap
    PalqiqiVectorEyeDisplay::emotion_name_maps_[] = {
        // 中性/平静类
        {"neutral", vector_eyes::Emotion::Normal},
        {"relaxed", vector_eyes::Emotion::Sleepy},
        {"sleepy", vector_eyes::Emotion::Sleepy},

        // 积极/开心类
        {"happy", vector_eyes::Emotion::Happy},
        {"laughing", vector_eyes::Emotion::Glee},
        {"funny", vector_eyes::Emotion::Glee},
        {"loving", vector_eyes::Emotion::Happy},
        {"confident", vector_eyes::Emotion::Normal},
        {"winking", vector_eyes::Emotion::Happy},
        {"cool", vector_eyes::Emotion::Skeptic},
        {"delicious", vector_eyes::Emotion::Glee},
        {"kissy", vector_eyes::Emotion::Happy},
        {"silly", vector_eyes::Emotion::Glee},

        // 悲伤类
        {"sad", vector_eyes::Emotion::Sad},
        {"crying", vector_eyes::Emotion::Sad},

        // 愤怒类
        {"angry", vector_eyes::Emotion::Angry},
        {"furious", vector_eyes::Emotion::Furious},

        // 惊讶类
        {"surprised", vector_eyes::Emotion::Surprised},
        {"shocked", vector_eyes::Emotion::Scared},

        // 思考/困惑类
        {"thinking", vector_eyes::Emotion::Skeptic},
        {"confused", vector_eyes::Emotion::Worried},
        {"embarrassed", vector_eyes::Emotion::Unimpressed},

        // 其他
        {"focused", vector_eyes::Emotion::Focused},
        {"annoyed", vector_eyes::Emotion::Annoyed},
        {"suspicious", vector_eyes::Emotion::Suspicious},
        {"awe", vector_eyes::Emotion::Awe},

        {nullptr, vector_eyes::Emotion::Normal} // 结束标记
};

PalqiqiVectorEyeDisplay::PalqiqiVectorEyeDisplay(esp_lcd_panel_io_handle_t panel_io,
                                           esp_lcd_panel_handle_t panel,
                                           int width, int height, int offset_x,
                                           int offset_y, bool mirror_x,
                                           bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                    mirror_x, mirror_y, swap_xy) {
  SetupCanvas();
  StartUpdateTimer();
}

PalqiqiVectorEyeDisplay::~PalqiqiVectorEyeDisplay() {
  StopUpdateTimer();
  if (face_) {
    delete face_;
    face_ = nullptr;
  }
  if (canvas_buf_) {
    lv_free(canvas_buf_);
    canvas_buf_ = nullptr;
  }
}

void PalqiqiVectorEyeDisplay::SetupCanvas() {
  DisplayLockGuard lock(static_cast<Display *>(this));

  // 删除原有的 emoji_label_ 和 chat_message_label_
  if (emoji_label_) {
    lv_obj_del(emoji_label_);
    emoji_label_ = nullptr;
  }
  if (chat_message_label_) {
    lv_obj_del(chat_message_label_);
  }
  if (content_) {
    lv_obj_del(content_);
  }

  // 创建内容容器
  content_ = lv_obj_create(container_);
  lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_size(content_, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_bg_opa(content_, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(content_, lv_color_black(), 0);
  lv_obj_set_style_border_width(content_, 0, 0);
  lv_obj_set_style_pad_all(content_, 0, 0);
  lv_obj_set_flex_grow(content_, 1);
  lv_obj_center(content_);

  // 创建 canvas 用于绘制眼睛
  int canvas_size = LV_HOR_RES;
  canvas_buf_ =
      (lv_color_t *)lv_malloc(canvas_size * canvas_size * sizeof(lv_color_t));

  if (canvas_buf_) {
    canvas_ = lv_canvas_create(content_);
    lv_canvas_set_buffer(canvas_, canvas_buf_, canvas_size, canvas_size,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_center(canvas_);
    lv_canvas_fill_bg(canvas_, lv_color_black(), LV_OPA_COVER);

    // 创建 VectorFace
    face_ = new vector_eyes::VectorFace(canvas_size, canvas_size, 80);
    face_->SetCanvas(canvas_);
    // 使用 Cozmo 风格的青色眼睛
    face_->SetEyeColor(lv_color_hex(0x00D4AA)); // Cozmo 青色
    face_->SetBackgroundColor(lv_color_black());

    ESP_LOGI(TAG, "矢量眼睛初始化完成，canvas大小: %dx%d", canvas_size,
             canvas_size);
  } else {
    ESP_LOGE(TAG, "无法分配 canvas 缓冲区");
  }

  // 创建一个隐藏的 emoji_label_ 以满足父类需求
  emoji_label_ = lv_label_create(content_);
  lv_label_set_text(emoji_label_, "");
  lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);

  // 创建聊天消息标签
  chat_message_label_ = lv_label_create(content_);
  lv_label_set_text(chat_message_label_, "");
  lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);
  lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
  lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
  lv_obj_set_style_border_width(chat_message_label_, 0, 0);
  lv_obj_set_style_bg_opa(chat_message_label_, LV_OPA_70, 0);
  lv_obj_set_style_pad_ver(chat_message_label_, 5, 0);
  lv_obj_align(chat_message_label_, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);

  // 注意：不调用 SetTheme，矢量眼睛使用固定的黑底白眼风格
}

void PalqiqiVectorEyeDisplay::StartUpdateTimer() {
  // 根据目标帧率创建更新定时器，默认 20Hz (50ms)
  uint32_t period_ms = 1000 / target_fps_;
  update_timer_ = lv_timer_create(UpdateTimerCallback, period_ms, this);
  ESP_LOGI(TAG, "🎯 启动更新定时器: %d FPS (周期: %lu ms)", target_fps_, (unsigned long)period_ms);
}

void PalqiqiVectorEyeDisplay::StopUpdateTimer() {
  if (update_timer_) {
    lv_timer_del(update_timer_);
    update_timer_ = nullptr;
  }
}

void PalqiqiVectorEyeDisplay::UpdateTimerCallback(lv_timer_t *timer) {
  auto *self =
      static_cast<PalqiqiVectorEyeDisplay *>(lv_timer_get_user_data(timer));
  if (self) {
    self->OnUpdate();
  }
}

void PalqiqiVectorEyeDisplay::OnUpdate() {
  if (!face_ || !canvas_)
    return;

  // 🎯 说话时保持正常帧率，不再跳帧
  // 通过 taskYIELD() 让出 CPU 时间给音频任务

  // 📊 记录渲染开始时间
  uint32_t render_start = lv_tick_get();

  // 🎯 优化：先在锁外更新动画状态（不涉及 LVGL 操作）
  // 检查演示模式和随机表情（这些只修改内部状态，不需要锁）
  if (demo_mode_) {
    CheckDemoMode();
  } else {
    CheckRandomEmotion();
  }

  // 更新动画状态（计算眨眼、视线等，不涉及 LVGL）
  face_->Update();

  // 🎯 只在绘制时持有锁，最小化锁持有时间
  {
    DisplayLockGuard lock(static_cast<Display *>(this));
    
    // 重绘眼睛
    face_->Draw();

    // 通知 LVGL canvas 已更新
    lv_obj_invalidate(canvas_);
  }
  // 锁在这里释放，让音频任务有机会获取锁

  // 📊 计算渲染耗时
  uint32_t render_end = lv_tick_get();
  last_render_time_ = render_end - render_start;
  
  // 📊 帧率统计：每秒计算一次实际帧率
  frame_count_++;
  if (render_end - last_fps_update_ >= 1000) {
    actual_fps_ = (float)frame_count_ * 1000.0f / (float)(render_end - last_fps_update_);
    // 只在调试时输出，减少日志开销
    // ESP_LOGI(TAG, "📊 FPS=%.1f, 渲染=%lums", actual_fps_, (unsigned long)last_render_time_);
    frame_count_ = 0;
    last_fps_update_ = render_end;
  }
  
  // ⚠️ 渲染时间超过30ms时输出警告（减少日志频率）
  static uint32_t last_warning_time = 0;
  if (last_render_time_ > 30 && (render_end - last_warning_time > 5000)) {
    ESP_LOGW(TAG, "⚠️ 渲染时间过长: %lums", (unsigned long)last_render_time_);
    last_warning_time = render_end;
  }
  
  // 🎯 渲染完成后让出 CPU，给音频任务处理时间
  taskYIELD();
}

void PalqiqiVectorEyeDisplay::CheckRandomEmotion() {
  if (!idle_mode_)
    return;

  // 只在设备待命状态下才随机变化表情
  auto &app = Application::GetInstance();
  if (app.GetDeviceState() != kDeviceStateIdle) {
    return;
  }

  uint32_t now = lv_tick_get();

  // 初始化
  if (next_emotion_interval_ == 0) {
    ScheduleNextEmotionChange();
    last_emotion_change_ = now;
    return;
  }

  // 检查是否到了变化时间
  if (now - last_emotion_change_ > next_emotion_interval_) {
    // 随机选择一个表情
    static const vector_eyes::Emotion idle_emotions[] = {
        vector_eyes::Emotion::Normal,
        vector_eyes::Emotion::Normal, // 增加Normal的权重
        vector_eyes::Emotion::Sleepy,     vector_eyes::Emotion::Skeptic,
        vector_eyes::Emotion::Suspicious, vector_eyes::Emotion::Focused,
    };

    int idx = rand() % (sizeof(idle_emotions) / sizeof(idle_emotions[0]));
    vector_eyes::Emotion new_emotion = idle_emotions[idx];

    if (new_emotion != current_emotion_) {
      current_emotion_ = new_emotion;
      face_->SetExpression(new_emotion);
      ESP_LOGI(TAG, "🎲 随机表情变化: %d", static_cast<int>(new_emotion));
    }

    last_emotion_change_ = now;
    ScheduleNextEmotionChange();
  }
}

void PalqiqiVectorEyeDisplay::CheckDemoMode() {
  if (!demo_mode_)
    return;

  uint32_t now = lv_tick_get();

  // 初始化演示
  if (demo_start_time_ == 0) {
    demo_start_time_ = now;
    demo_emotion_index_ = 0;
    ESP_LOGI(TAG, "🎭 开始表情演示模式");
  }

  // 每2秒切换一个表情
  if (now - demo_start_time_ > 2000) {
    static const vector_eyes::Emotion all_emotions[] = {
        vector_eyes::Emotion::Normal,      // 0
        vector_eyes::Emotion::Happy,       // 1
        vector_eyes::Emotion::Glee,        // 2
        vector_eyes::Emotion::Sad,         // 3
        vector_eyes::Emotion::Worried,     // 4
        vector_eyes::Emotion::Focused,     // 5
        vector_eyes::Emotion::Annoyed,     // 6
        vector_eyes::Emotion::Surprised,   // 7
        vector_eyes::Emotion::Skeptic,     // 8
        vector_eyes::Emotion::Frustrated,  // 9
        vector_eyes::Emotion::Unimpressed, // 10
        vector_eyes::Emotion::Sleepy,      // 11
        vector_eyes::Emotion::Suspicious,  // 12
        vector_eyes::Emotion::Squint,      // 13
        vector_eyes::Emotion::Angry,       // 14
        vector_eyes::Emotion::Furious,     // 15
        vector_eyes::Emotion::Scared,      // 16
        vector_eyes::Emotion::Awe          // 17
    };
    static const char *emotion_names[] = {
        "Normal",      "Happy",   "Glee",       "Sad",     "Worried",
        "Focused",     "Annoyed", "Surprised",  "Skeptic", "Frustrated",
        "Unimpressed", "Sleepy",  "Suspicious", "Squint",  "Angry",
        "Furious",     "Scared",  "Awe"};

    if (demo_emotion_index_ < 18) {
      vector_eyes::Emotion emotion = all_emotions[demo_emotion_index_];
      face_->SetExpression(emotion);
      ESP_LOGI(TAG, "🎭 演示表情 %d/18: %s", demo_emotion_index_ + 1,
               emotion_names[demo_emotion_index_]);
      demo_emotion_index_++;
      demo_start_time_ = now;
    } else {
      // 演示结束，进入正常模式
      demo_mode_ = false;
      idle_mode_ = true;
      face_->SetExpression(vector_eyes::Emotion::Normal);
      ESP_LOGI(TAG, "🎭 表情演示完成，进入正常模式");
    }
  }
}

void PalqiqiVectorEyeDisplay::ScheduleNextEmotionChange() {
  // 8-15秒随机间隔
  next_emotion_interval_ = 8000 + (rand() % 7000);
}

vector_eyes::Emotion PalqiqiVectorEyeDisplay::MapEmotionName(const char *name) {
  if (!name)
    return vector_eyes::Emotion::Normal;

  for (const auto &map : emotion_name_maps_) {
    if (map.name && strcmp(map.name, name) == 0) {
      return map.emotion;
    }
  }

  ESP_LOGW(TAG, "未知表情名称: %s，使用默认", name);
  return vector_eyes::Emotion::Normal;
}

void PalqiqiVectorEyeDisplay::SetEmotion(const char *emotion) {
  if (!emotion || !face_)
    return;

  DisplayLockGuard lock(static_cast<Display *>(this));

  vector_eyes::Emotion mapped = MapEmotionName(emotion);

  // 如果是 neutral，进入空闲模式，允许随机表情
  // 如果是其他表情，退出空闲模式
  if (mapped == vector_eyes::Emotion::Normal) {
    idle_mode_ = true;
  } else {
    idle_mode_ = false;
    current_emotion_ = mapped;
  }

  face_->SetExpression(mapped);

  // 表情名称映射
  static const char *emotion_names[] = {
      "Normal",      "Happy",   "Glee",       "Sad",     "Worried",
      "Focused",     "Annoyed", "Surprised",  "Skeptic", "Frustrated",
      "Unimpressed", "Sleepy",  "Suspicious", "Squint",  "Angry",
      "Furious",     "Scared",  "Awe"};
  const char *mapped_name = (static_cast<int>(mapped) < 18)
                                ? emotion_names[static_cast<int>(mapped)]
                                : "Unknown";

  ESP_LOGI(TAG, "🎭 表情变化: '%s' -> %s (%d), 空闲模式: %s", emotion,
           mapped_name, static_cast<int>(mapped), idle_mode_ ? "是" : "否");
}

void PalqiqiVectorEyeDisplay::SetChatMessage(const char *role,
                                          const char *content) {
  DisplayLockGuard lock(static_cast<Display *>(this));

  if (chat_message_label_ == nullptr)
    return;

  if (content == nullptr || strlen(content) == 0) {
    lv_obj_add_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_label_set_text(chat_message_label_, content);
  lv_obj_remove_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);

  ESP_LOGI(TAG, "设置聊天消息 [%s]: %s", role, content);
}

void PalqiqiVectorEyeDisplay::Blink() {
  if (face_) {
    face_->Blink();
  }
}

void PalqiqiVectorEyeDisplay::LookAt(float x, float y) {
  if (face_) {
    face_->LookAt(x, y);
  }
}

void PalqiqiVectorEyeDisplay::SetEyeColor(uint32_t color_hex) {
  if (face_) {
    face_->SetEyeColor(lv_color_hex(color_hex));
  }
}

void PalqiqiVectorEyeDisplay::SetTheme(Theme *theme) {
  // 矢量眼睛使用固定的黑底白眼风格，不需要主题切换
  // 只保存主题设置，不应用到UI元素
  DisplayLockGuard lock(static_cast<Display *>(this));

  // 只调用基类的Display::SetTheme来保存设置，跳过LcdDisplay的UI更新
  Display::SetTheme(theme);

  ESP_LOGI(TAG, "矢量眼睛模式：跳过主题UI更新");
}

bool PalqiqiVectorEyeDisplay::SetTargetFrameRate(uint8_t fps) {
  // 验证帧率范围 10-30 FPS (Requirements 4.1)
  if (fps < kMinFps || fps > kMaxFps) {
    ESP_LOGW(TAG, "⚠️ 帧率设置超出范围: %d FPS (有效范围: %d-%d)", 
             fps, kMinFps, kMaxFps);
    return false;
  }
  
  target_fps_ = fps;
  
  // 计算新的 timer 周期 (ms) = 1000 / fps
  uint32_t period_ms = 1000 / fps;
  
  // 更新 LVGL timer 周期
  if (update_timer_) {
    lv_timer_set_period(update_timer_, period_ms);
    ESP_LOGI(TAG, "🎯 帧率设置: %d FPS (周期: %lu ms)", fps, (unsigned long)period_ms);
  } else {
    ESP_LOGW(TAG, "⚠️ 更新定时器未初始化，帧率设置将在下次启动时生效");
  }
  
  return true;
}
