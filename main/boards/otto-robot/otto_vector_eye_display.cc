/**
 * @file otto_vector_eye_display.cc
 * @brief Otto机器人矢量眼睛显示类实现
 */

#include "otto_vector_eye_display.h"
#include "display/lvgl_display/lvgl_theme.h"

#include <esp_log.h>
#include <cstring>

#define TAG "OttoVectorEyeDisplay"

// 表情名称映射表 - 将现有表情名映射到矢量表情
const OttoVectorEyeDisplay::EmotionNameMap OttoVectorEyeDisplay::emotion_name_maps_[] = {
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

    {nullptr, vector_eyes::Emotion::Normal}  // 结束标记
};

OttoVectorEyeDisplay::OttoVectorEyeDisplay(
    esp_lcd_panel_io_handle_t panel_io,
    esp_lcd_panel_handle_t panel, 
    int width, int height,
    int offset_x, int offset_y,
    bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                    mirror_x, mirror_y, swap_xy) {
    SetupCanvas();
    StartUpdateTimer();
}

OttoVectorEyeDisplay::~OttoVectorEyeDisplay() {
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

void OttoVectorEyeDisplay::SetupCanvas() {
    DisplayLockGuard lock(static_cast<Display*>(this));

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
    canvas_buf_ = (lv_color_t*)lv_malloc(canvas_size * canvas_size * sizeof(lv_color_t));
    
    if (canvas_buf_) {
        canvas_ = lv_canvas_create(content_);
        lv_canvas_set_buffer(canvas_, canvas_buf_, canvas_size, canvas_size, LV_COLOR_FORMAT_RGB565);
        lv_obj_center(canvas_);
        lv_canvas_fill_bg(canvas_, lv_color_black(), LV_OPA_COVER);

        // 创建 VectorFace
        face_ = new vector_eyes::VectorFace(canvas_size, canvas_size, 80);
        face_->SetCanvas(canvas_);
        face_->SetEyeColor(lv_color_white());
        face_->SetBackgroundColor(lv_color_black());

        ESP_LOGI(TAG, "矢量眼睛初始化完成，canvas大小: %dx%d", canvas_size, canvas_size);
    } else {
        ESP_LOGE(TAG, "无法分配 canvas 缓冲区");
    }

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

    // 使用深色主题
    auto& theme_manager = LvglThemeManager::GetInstance();
    auto dark_theme = theme_manager.GetTheme("dark");
    if (dark_theme != nullptr) {
        LcdDisplay::SetTheme(dark_theme);
    }
}

void OttoVectorEyeDisplay::StartUpdateTimer() {
    // 创建 30Hz 更新定时器
    update_timer_ = lv_timer_create(UpdateTimerCallback, 33, this);
}

void OttoVectorEyeDisplay::StopUpdateTimer() {
    if (update_timer_) {
        lv_timer_del(update_timer_);
        update_timer_ = nullptr;
    }
}

void OttoVectorEyeDisplay::UpdateTimerCallback(lv_timer_t* timer) {
    auto* self = static_cast<OttoVectorEyeDisplay*>(lv_timer_get_user_data(timer));
    if (self) {
        self->OnUpdate();
    }
}

void OttoVectorEyeDisplay::OnUpdate() {
    if (!face_ || !canvas_) return;

    DisplayLockGuard lock(static_cast<Display*>(this));
    
    // 更新动画状态
    face_->Update();
    
    // 重绘眼睛
    face_->Draw();
    
    // 通知 LVGL canvas 已更新
    lv_obj_invalidate(canvas_);
}

vector_eyes::Emotion OttoVectorEyeDisplay::MapEmotionName(const char* name) {
    if (!name) return vector_eyes::Emotion::Normal;

    for (const auto& map : emotion_name_maps_) {
        if (map.name && strcmp(map.name, name) == 0) {
            return map.emotion;
        }
    }

    ESP_LOGW(TAG, "未知表情名称: %s，使用默认", name);
    return vector_eyes::Emotion::Normal;
}

void OttoVectorEyeDisplay::SetEmotion(const char* emotion) {
    if (!emotion || !face_) return;

    DisplayLockGuard lock(static_cast<Display*>(this));

    vector_eyes::Emotion mapped = MapEmotionName(emotion);
    face_->SetExpression(mapped);

    ESP_LOGI(TAG, "设置表情: %s -> %d", emotion, static_cast<int>(mapped));
}

void OttoVectorEyeDisplay::SetChatMessage(const char* role, const char* content) {
    DisplayLockGuard lock(static_cast<Display*>(this));
    
    if (chat_message_label_ == nullptr) return;

    if (content == nullptr || strlen(content) == 0) {
        lv_obj_add_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(chat_message_label_, content);
    lv_obj_remove_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "设置聊天消息 [%s]: %s", role, content);
}

void OttoVectorEyeDisplay::Blink() {
    if (face_) {
        face_->Blink();
    }
}

void OttoVectorEyeDisplay::LookAt(float x, float y) {
    if (face_) {
        face_->LookAt(x, y);
    }
}

void OttoVectorEyeDisplay::SetEyeColor(uint32_t color_hex) {
    if (face_) {
        face_->SetEyeColor(lv_color_hex(color_hex));
    }
}
