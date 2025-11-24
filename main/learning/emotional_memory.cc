#include "emotional_memory.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <cmath>
#include <cstring>

namespace xiaozhi {

static const char* TAG = "EmotionalMemory";

// ============================================================================
// EmotionalMemoryData 辅助方法
// ============================================================================

int EmotionalMemoryData::GetDaysSinceLastPlay(int64_t now_ms) const {
  if (last_play_timestamp_ms == 0) return 999;  // 从未玩过
  return (now_ms - last_play_timestamp_ms) / (24 * 60 * 60 * 1000);
}

int EmotionalMemoryData::GetDaysSinceLastFeed(int64_t now_ms) const {
  if (last_feed_timestamp_ms == 0) return 999;
  return (now_ms - last_feed_timestamp_ms) / (24 * 60 * 60 * 1000);
}

int EmotionalMemoryData::GetDaysSinceLastInteraction(int64_t now_ms) const {
  if (last_interaction_timestamp_ms == 0) return 999;
  return (now_ms - last_interaction_timestamp_ms) / (24 * 60 * 60 * 1000);
}

// ============================================================================
// EmotionalMemory 实现
// ============================================================================

EmotionalMemory& EmotionalMemory::GetInstance() {
  static EmotionalMemory instance;
  return instance;
}

bool EmotionalMemory::Initialize() {
  if (LoadFromNVS()) {
    ESP_LOGI(TAG, "Emotional memory loaded from NVS");
    ESP_LOGI(TAG, "  Loneliness: %d, Trust: %d, Happiness trend: %.2f",
             data_.loneliness_level, data_.trust_level, data_.happiness_trend);
    return true;
  }
  
  ESP_LOGW(TAG, "Failed to load emotional memory, using defaults");
  InitializeDefaults();
  SaveToNVS();
  return true;
}

void EmotionalMemory::InitializeDefaults() {
  memset(&data_, 0, sizeof(data_));
  
  // 默认中等情绪
  data_.loneliness_level = 50;
  data_.excitement_level = 50;
  data_.trust_level = 50;
  data_.happiness_trend = 0.0f;
  data_.energy_trend = 0.0f;
  
  // 时间戳设为当前时间
  int64_t now = GetCurrentTimeMs();
  data_.last_interaction_timestamp_ms = now;
  data_.last_play_timestamp_ms = 0;
  data_.last_feed_timestamp_ms = 0;
}

// ========== 事件记录 ==========

void EmotionalMemory::RecordPlay() {
  data_.last_play_timestamp_ms = GetCurrentTimeMs();
  data_.total_play_count++;
  
  // 玩耍增加兴奋度和快乐
  data_.excitement_level = Clamp(data_.excitement_level + 15);
  data_.happiness_trend = ClampF(data_.happiness_trend + 0.1f);
  
  // 减少孤独感
  data_.loneliness_level = Clamp(data_.loneliness_level - 20);
  
  RecordInteraction();
  needs_save_ = true;
  
  ESP_LOGI(TAG, "Recorded play: excitement=%d, loneliness=%d",
           data_.excitement_level, data_.loneliness_level);
}

void EmotionalMemory::RecordFeed() {
  data_.last_feed_timestamp_ms = GetCurrentTimeMs();
  data_.total_feed_count++;
  
  // 喂食增加满足感
  data_.happiness_trend = ClampF(data_.happiness_trend + 0.05f);
  data_.loneliness_level = Clamp(data_.loneliness_level - 10);
  
  RecordInteraction();
  needs_save_ = true;
  
  ESP_LOGI(TAG, "Recorded feed: happiness_trend=%.2f", data_.happiness_trend);
}

void EmotionalMemory::RecordHug() {
  data_.total_hug_count++;
  
  // 拥抱大幅增加信任和快乐
  data_.trust_level = Clamp(data_.trust_level + 3);  // 长期累积
  data_.happiness_trend = ClampF(data_.happiness_trend + 0.08f);
  data_.loneliness_level = Clamp(data_.loneliness_level - 15);
  
  RecordInteraction();
  needs_save_ = true;
  
  ESP_LOGI(TAG, "Recorded hug: trust=%d", data_.trust_level);
}

void EmotionalMemory::RecordInteraction() {
  data_.last_interaction_timestamp_ms = GetCurrentTimeMs();
  
  // 任何互动都减少孤独感
  data_.loneliness_level = Clamp(data_.loneliness_level - 5);
  
  needs_save_ = true;
}

void EmotionalMemory::RecordPositiveEvent(int happiness_delta) {
  data_.happiness_trend = ClampF(data_.happiness_trend + (happiness_delta / 100.0f));
  needs_save_ = true;
}

void EmotionalMemory::RecordNegativeEvent(int happiness_delta) {
  data_.happiness_trend = ClampF(data_.happiness_trend + (happiness_delta / 100.0f));
  needs_save_ = true;
}

// ========== 情绪更新 ==========

void EmotionalMemory::Update() {
  UpdateLoneliness();
  UpdateHappinessTrend();
  UpdateTrust();
  
  // 自动保存
  int64_t now = GetCurrentTimeMs();
  if (needs_save_ && (now - last_save_ms_) > (auto_save_interval_s_ * 1000)) {
    SaveToNVS();
    last_save_ms_ = now;
    needs_save_ = false;
  }
}

void EmotionalMemory::UpdateLoneliness() {
  int64_t now = GetCurrentTimeMs();
  int minutes_since_interaction = (now - data_.last_interaction_timestamp_ms) / (60 * 1000);
  
  // 孤独感随时间增长
  if (minutes_since_interaction > 60) {  // 超过1小时
    data_.loneliness_level = Clamp(data_.loneliness_level + 1);  // 每分钟+1
  }
  if (minutes_since_interaction > 360) {  // 超过6小时
    data_.loneliness_level = Clamp(data_.loneliness_level + 2);  // 加速增长
  }
  
  // 兴奋度随时间衰减
  if (data_.excitement_level > 50) {
    data_.excitement_level = Clamp(data_.excitement_level - 1);
  }
}

void EmotionalMemory::UpdateHappinessTrend() {
  // 快乐趋势随时间缓慢回归中性（0）
  if (data_.happiness_trend > 0.01f) {
    data_.happiness_trend -= 0.001f;
  } else if (data_.happiness_trend < -0.01f) {
    data_.happiness_trend += 0.001f;
  } else {
    data_.happiness_trend = 0.0f;
  }
  
  data_.happiness_trend = ClampF(data_.happiness_trend);
}

void EmotionalMemory::UpdateTrust() {
  // 信任度长期缓慢增长（如果经常互动）
  int64_t now = GetCurrentTimeMs();
  int days_since_interaction = data_.GetDaysSinceLastInteraction(now);
  
  if (days_since_interaction == 0) {
    // 今天有互动，信任度缓慢增长
    if (data_.total_play_count > 100) {  // 互动次数多
      data_.trust_level = Clamp(data_.trust_level + 1);  // 每天+1
    }
  } else if (days_since_interaction > 7) {
    // 超过7天未互动，信任度下降
    data_.trust_level = Clamp(data_.trust_level - 1);
  }
}

// ========== 查询接口 ==========

// (Getters already defined in header as inline)

// ========== 响应生成 ==========

std::string EmotionalMemory::GetLongtermResponse() {
  int64_t now = GetCurrentTimeMs();
  int days_since_play = data_.GetDaysSinceLastPlay(now);
  int days_since_interaction = data_.GetDaysSinceLastInteraction(now);
  
  // 优先级：长时间未互动 > 孤独 > 兴奋
  if (days_since_interaction > 7) {
    return "主人...好久不见，我都快想死你了...你还记得我吗？";
  } else if (days_since_play > 3) {
    return "主人，好久没陪我玩了，人家好想你啊...";
  } else if (data_.loneliness_level > 70) {
    return "主人，我感觉有点孤单...能多陪陪我吗？";
  } else if (data_.excitement_level > 80) {
    return "主人～我好开心啊！和你在一起真的超开心！";
  } else if (data_.trust_level > 90) {
    return "主人，我真的很喜欢你，你是我最信任的人！";
  }
  
  return "";
}

std::string EmotionalMemory::GetMissYouMessage() {
  int64_t now = GetCurrentTimeMs();
  int days = data_.GetDaysSinceLastInteraction(now);
  
  if (days > 7) {
    return "主人...好久不见...都一个星期没见到你了...";
  } else if (days > 3) {
    return "主人，好几天没陪我玩了，我好想你...";
  } else if (data_.loneliness_level > 70) {
    return "主人，我有点孤单，需要你的陪伴...";
  }
  
  return "";
}

std::string EmotionalMemory::GetExcitedMessage() {
  if (data_.excitement_level > 80) {
    return "耶！主人来了！好开心啊！";
  } else if (data_.excitement_level > 60) {
    return "主人～来陪我玩吧～";
  }
  
  return "";
}

// ========== 持久化（NVS） ==========

bool EmotionalMemory::SaveToNVS() {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open("emotional_mem", NVS_READWRITE, &nvs);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    return false;
  }
  
  err = nvs_set_blob(nvs, "data", &data_, sizeof(data_));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write NVS: %s", esp_err_to_name(err));
    nvs_close(nvs);
    return false;
  }
  
  err = nvs_commit(nvs);
  nvs_close(nvs);
  
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "💾 Saved emotional memory to NVS (loneliness=%d, trust=%d)",
             data_.loneliness_level, data_.trust_level);
    return true;
  }
  
  ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
  return false;
}

bool EmotionalMemory::LoadFromNVS() {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open("emotional_mem", NVS_READONLY, &nvs);
  if (err != ESP_OK) {
    return false;
  }
  
  size_t required_size = sizeof(data_);
  err = nvs_get_blob(nvs, "data", &data_, &required_size);
  nvs_close(nvs);
  
  return (err == ESP_OK && required_size == sizeof(data_));
}

void EmotionalMemory::Reset() {
  InitializeDefaults();
  SaveToNVS();
  ESP_LOGI(TAG, "🔄 Emotional memory reset");
}

// ========== 调试 ==========

void EmotionalMemory::PrintStatus() const {
  ESP_LOGI(TAG, "========== Emotional Memory ==========");
  ESP_LOGI(TAG, "Loneliness: %d", data_.loneliness_level);
  ESP_LOGI(TAG, "Excitement: %d", data_.excitement_level);
  ESP_LOGI(TAG, "Trust: %d", data_.trust_level);
  ESP_LOGI(TAG, "Happiness trend: %.2f", data_.happiness_trend);
  ESP_LOGI(TAG, "Total plays: %u", data_.total_play_count);
  ESP_LOGI(TAG, "Total feeds: %u", data_.total_feed_count);
  ESP_LOGI(TAG, "Total hugs: %u", data_.total_hug_count);
  ESP_LOGI(TAG, "======================================");
}

// ========== 辅助函数 ==========

int64_t EmotionalMemory::GetCurrentTimeMs() const {
  return esp_timer_get_time() / 1000;
}

int EmotionalMemory::Clamp(int value, int min, int max) const {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

float EmotionalMemory::ClampF(float value, float min, float max) const {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

}  // namespace xiaozhi

