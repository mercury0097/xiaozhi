#include "user_profile.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <cstring>
#include <ctime>

namespace xiaozhi {

static const char* TAG = "UserProfile";

// ============================================================================
// UserProfile 实现
// ============================================================================

UserProfile& UserProfile::GetInstance() {
  static UserProfile instance;
  return instance;
}

bool UserProfile::Initialize() {
  // 尝试从 NVS 加载
  if (LoadFromNVS()) {
    ESP_LOGI(TAG, "✅ Loaded user profile from NVS");
    PrintStats();
    return true;
  }

  // 加载失败，使用默认值
  ESP_LOGW(TAG, "⚠️  Failed to load profile, using defaults");
  InitializeDefaults();
  SaveToNVS();  // 保存默认值
  return true;
}

void UserProfile::InitializeDefaults() {
  memset(&data_, 0, sizeof(data_));
  
  // 设置默认值
  data_.interaction_count_7d = 0;
  data_.avg_session_duration_s = 30;  // 默认30秒
  data_.created_ms = esp_timer_get_time() / 1000;
  data_.last_update_ms = data_.created_ms;
  
  needs_save_ = true;
}

// ========== 数据记录 ==========

void UserProfile::RecordInteraction(const char* topic, uint32_t duration_ms) {
  // 增加互动计数
  data_.interaction_count_7d++;
  
  // 更新平均会话时长（简单移动平均）
  uint32_t duration_s = duration_ms / 1000;
  uint32_t old_avg = data_.avg_session_duration_s;
  if (data_.avg_session_duration_s == 0) {
    data_.avg_session_duration_s = duration_s;
  } else {
    // 加权平均（80%旧值 + 20%新值）
    data_.avg_session_duration_s = 
        (data_.avg_session_duration_s * 4 + duration_s) / 5;
  }
  
  // 增加话题计数
  IncrementTopicCount(topic);
  
  // 更新时间活跃度
  UpdateTimeActivity();
  
  // 更新时间戳
  data_.last_update_ms = esp_timer_get_time() / 1000;
  
  // 🎯 关键修复：立即保存到 NVS（不再等待5分钟）
  SaveToNVS();
  
  // 🎯 详细日志展示学习过程
  ESP_LOGI(TAG, "🧠 Learning: topic=%s, duration=%us", 
           topic ? topic : "unknown", duration_s);
  ESP_LOGI(TAG, "  📈 Total interactions: %u → %u", 
           data_.interaction_count_7d - 1, data_.interaction_count_7d);
  ESP_LOGI(TAG, "  ⏱️  Avg session: %us → %us", 
           old_avg, data_.avg_session_duration_s);
  ESP_LOGI(TAG, "  ⭐ Favorite topic: %s", GetFavoriteTopic());
  ESP_LOGI(TAG, "  💾 Immediately saved to NVS");
}

void UserProfile::RecordFeedback(bool is_positive) {
  if (is_positive) {
    data_.positive_feedback_count++;
  } else {
    data_.negative_feedback_count++;
  }
  
  data_.last_update_ms = esp_timer_get_time() / 1000;
  needs_save_ = true;
  
  ESP_LOGI(TAG, "Recorded feedback: %s (total: +%u / -%u, ratio: %u%%)",
           is_positive ? "positive" : "negative",
           data_.positive_feedback_count,
           data_.negative_feedback_count,
           data_.GetPositiveRatio());
}

void UserProfile::UpdateTimeActivity() {
  // 获取当前小时
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  int hour = timeinfo.tm_hour;
  
  // 增加该小时的活跃度（上限255）
  if (data_.active_hours[hour] < 255) {
    data_.active_hours[hour]++;
  }
}

void UserProfile::IncrementTopicCount(const char* topic) {
  if (!topic) {
    data_.topic_other++;
    return;
  }
  
  // 简单字符串匹配
  if (strstr(topic, "weather") || strstr(topic, "天气")) {
    data_.topic_weather++;
  } else if (strstr(topic, "story") || strstr(topic, "故事")) {
    data_.topic_story++;
  } else if (strstr(topic, "pet") || strstr(topic, "宠物")) {
    data_.topic_pet++;
  } else if (strstr(topic, "smart_home") || strstr(topic, "智能家居") || 
             strstr(topic, "control") || strstr(topic, "控制")) {
    data_.topic_smart_home++;
  } else if (strstr(topic, "chat") || strstr(topic, "聊天")) {
    data_.topic_chat++;
  } else {
    data_.topic_other++;
  }
}

const char* UserProfile::GetFavoriteTopic() const {
  uint16_t max_count = 0;
  const char* favorite = "chat";
  
  if (data_.topic_weather > max_count) {
    max_count = data_.topic_weather;
    favorite = "weather";
  }
  if (data_.topic_story > max_count) {
    max_count = data_.topic_story;
    favorite = "story";
  }
  if (data_.topic_pet > max_count) {
    max_count = data_.topic_pet;
    favorite = "pet";
  }
  if (data_.topic_smart_home > max_count) {
    max_count = data_.topic_smart_home;
    favorite = "smart_home";
  }
  if (data_.topic_chat > max_count) {
    favorite = "chat";
  }
  
  return favorite;
}

// ========== 持久化（NVS） ==========

bool UserProfile::SaveToNVS() {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open("user_profile", NVS_READWRITE, &nvs);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "❌ Failed to open NVS: %s", esp_err_to_name(err));
    return false;
  }
  
  // 保存所有字段（使用二进制 blob 存储整个结构体，更高效）
  err = nvs_set_blob(nvs, "profile_data", &data_, sizeof(data_));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "❌ Failed to write profile data: %s", esp_err_to_name(err));
    nvs_close(nvs);
    return false;
  }
  
  err = nvs_commit(nvs);
  nvs_close(nvs);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "❌ Failed to commit NVS: %s", esp_err_to_name(err));
    return false;
  }
  
  last_save_ms_ = esp_timer_get_time() / 1000;
  needs_save_ = false;
  
  ESP_LOGI(TAG, "💾 Saved user profile to NVS (size: %u bytes, 7d互动: %u次)", 
           sizeof(data_), data_.interaction_count_7d);
  return true;
}

bool UserProfile::LoadFromNVS() {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open("user_profile", NVS_READONLY, &nvs);
  if (err != ESP_OK) {
    ESP_LOGD(TAG, "Profile not found in NVS (first run): %s", esp_err_to_name(err));
    return false;
  }
  
  // 读取二进制 blob
  size_t required_size = sizeof(data_);
  err = nvs_get_blob(nvs, "profile_data", &data_, &required_size);
  nvs_close(nvs);
  
  if (err != ESP_OK || required_size != sizeof(data_)) {
    ESP_LOGW(TAG, "Failed to read profile data: %s", esp_err_to_name(err));
    return false;
  }
  
  return true;
}

void UserProfile::Reset() {
  InitializeDefaults();
  SaveToNVS();
  ESP_LOGI(TAG, "🔄 User profile reset");
}

// ========== 辅助功能 ==========

void UserProfile::PrintStats() const {
  ESP_LOGI(TAG, "========== User Profile Stats ==========");
  ESP_LOGI(TAG, "Interactions (7d): %lu", data_.interaction_count_7d);
  ESP_LOGI(TAG, "Avg session: %lus", data_.avg_session_duration_s);
  ESP_LOGI(TAG, "Most active hour: %u:00", data_.GetMostActiveHour());
  ESP_LOGI(TAG, "Favorite topic: %s", GetFavoriteTopic());
  ESP_LOGI(TAG, "Positive ratio: %u%%", data_.GetPositiveRatio());
  ESP_LOGI(TAG, "Topics: weather=%u story=%u pet=%u home=%u chat=%u other=%u",
           data_.topic_weather, data_.topic_story, data_.topic_pet,
           data_.topic_smart_home, data_.topic_chat, data_.topic_other);
  ESP_LOGI(TAG, "========================================");
}

void UserProfile::SetAutoSaveInterval(uint32_t seconds) {
  auto_save_interval_s_ = seconds;
  ESP_LOGI(TAG, "Auto-save interval set to %lus", seconds);
}

void UserProfile::CheckAutoSave() {
  if (!needs_save_) {
    return;
  }
  
  int64_t now_ms = esp_timer_get_time() / 1000;
  int64_t elapsed_s = (now_ms - last_save_ms_) / 1000;
  
  if (elapsed_s >= auto_save_interval_s_) {
    SaveToNVS();
    ESP_LOGD(TAG, "💾 Auto-saved user profile");
  }
}

}  // namespace xiaozhi


