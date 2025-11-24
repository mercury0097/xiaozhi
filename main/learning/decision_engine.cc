#include "decision_engine.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_random.h>
#include <ctime>
#include <cmath>

namespace xiaozhi {

static const char* TAG = "DecisionEngine";

// ============================================================================
// DecisionEngine 实现
// ============================================================================

DecisionEngine& DecisionEngine::GetInstance() {
  static DecisionEngine instance;
  return instance;
}

bool DecisionEngine::Initialize() {
  profile_ = &UserProfile::GetInstance();
  
  ESP_LOGI(TAG, "Decision engine initialized");
  return true;
}

// ========== 概率决策 ==========

float DecisionEngine::GetProactiveProbability() {
  if (!profile_) return 0.1f;
  
  const auto& data = profile_->GetData();
  
  // 基础概率
  float prob = 0.1f;
  
  // 因素1：互动频率（高频用户 → 更主动）
  if (data.interaction_count_7d > 50) {
    prob += 0.2f;  // +20%
  } else if (data.interaction_count_7d > 20) {
    prob += 0.1f;  // +10%
  }
  
  // 因素2：活跃时段（当前是用户活跃时段 → 更主动）
  int current_hour = GetCurrentHour();
  if (data.active_hours[current_hour] > 10) {
    prob += 0.15f;  // +15%
  }
  
  // 因素3：正面反馈（用户反馈好 → 更主动）
  uint8_t positive_ratio = data.GetPositiveRatio();
  if (positive_ratio > 70) {
    prob += 0.1f;  // +10%
  } else if (positive_ratio < 30) {
    prob -= 0.1f;  // -10%（用户可能不喜欢主动问候）
  }
  
  // 因素4：长时间未互动（> 6小时 → 稍微降低）
  int64_t idle_s = GetSecondsSinceLastInteraction();
  if (idle_s > 6 * 3600) {
    prob -= 0.05f;  // -5%（可能用户在忙）
  }
  
  // 限制范围 0.0 ~ 0.6（最高60%主动率）
  prob = fmaxf(0.0f, fminf(prob, 0.6f));
  
  return prob;
}

int DecisionEngine::GetPetReminderInterval() {
  if (!profile_) return 120;  // 默认2小时
  
  const auto& data = profile_->GetData();
  
  // 基于用户养宠频率调整
  if (data.topic_pet > 100) {
    return 30;   // 高频养宠用户：30分钟提醒一次
  } else if (data.topic_pet > 50) {
    return 60;   // 中频用户：1小时
  } else if (data.topic_pet > 20) {
    return 90;   // 低频用户：1.5小时
  } else {
    return 120;  // 很少养宠：2小时
  }
}

int DecisionEngine::GetEmotionLiveness() {
  // 固定返回中等活跃度，不再自适应
  return 50;
}

// ========== 情境感知 ==========

DecisionEngine::Context DecisionEngine::GetCurrentContext() {
  int hour = GetCurrentHour();
  
  // 时间情境
  if (hour >= 6 && hour < 9) {
    return CONTEXT_MORNING;
  } else if (hour >= 9 && hour < 18) {
    return CONTEXT_WORK_TIME;
  } else if (hour >= 18 && hour < 22) {
    return CONTEXT_EVENING;
  } else {
    return CONTEXT_NIGHT;
  }
  
  // TODO: 可扩展电量检测、长时间空闲检测
}

const char* DecisionEngine::GetGreetingByContext(Context ctx) {
  if (!profile_) {
    return "你好！";
  }
  
  // 判断是否高频用户
  bool is_frequent = profile_->GetInteractionCount7d() > 30;
  
  switch (ctx) {
    case CONTEXT_MORNING:
      return is_frequent ? "早呀！睡得好吗？" : "早上好！";
      
    case CONTEXT_WORK_TIME:
      return is_frequent ? "休息一下？来聊聊天吧~" : "有什么可以帮你的吗？";
      
    case CONTEXT_EVENING:
      return is_frequent ? "晚上好呀，今天过得怎么样？" : "晚上好！";
      
    case CONTEXT_NIGHT:
      return "这么晚还没睡呀？";
      
    case CONTEXT_LOW_BATTERY:
      return "电量有点低了，我可能要休息一会儿";
      
    case CONTEXT_LONG_IDLE:
      return is_frequent ? "好久不见，想我了吗？" : "你好，需要帮助吗？";
      
    default:
      return "你好！";
  }
}

// ========== 个性化推荐 ==========

const char* DecisionEngine::RecommendTopic() {
  if (!profile_) {
    return "要不要聊聊天？";
  }
  
  const char* favorite = profile_->GetFavoriteTopic();
  
  if (strcmp(favorite, "weather") == 0) {
    return "要不要查查今天天气？";
  } else if (strcmp(favorite, "story") == 0) {
    return "要不要听个故事？";
  } else if (strcmp(favorite, "pet") == 0) {
    return "去看看宠物吧？";
  } else if (strcmp(favorite, "smart_home") == 0) {
    return "需要控制家里的设备吗？";
  } else {
    return "要不要聊聊天？";
  }
}

bool DecisionEngine::ShouldRemindPet() {
  int64_t now_ms = esp_timer_get_time() / 1000;
  int interval_min = GetPetReminderInterval();
  int64_t interval_ms = interval_min * 60 * 1000;
  
  if ((now_ms - last_pet_reminder_ms_) >= interval_ms) {
    last_pet_reminder_ms_ = now_ms;
    return true;
  }
  
  return false;
}

bool DecisionEngine::ShouldGreetProactively() {
  // 最小间隔：10分钟（防止太频繁）
  int64_t now_ms = esp_timer_get_time() / 1000;
  if ((now_ms - last_proactive_greeting_ms_) < 10 * 60 * 1000) {
    return false;
  }
  
  // 概率判断
  float prob = GetProactiveProbability();
  float rand_val = RandomFloat();
  
  if (rand_val < prob) {
    last_proactive_greeting_ms_ = now_ms;
    ESP_LOGI(TAG, "Proactive greeting triggered (prob=%.2f, rand=%.2f)", 
             prob, rand_val);
    return true;
  }
  
  return false;
}

// ========== 辅助功能 ==========

void DecisionEngine::PrintDecisionParams() {
  ESP_LOGI(TAG, "========== Decision Params ==========");
  ESP_LOGI(TAG, "Proactive prob: %.2f", GetProactiveProbability());
  ESP_LOGI(TAG, "Pet reminder interval: %d min", GetPetReminderInterval());
  ESP_LOGI(TAG, "Current context: %d", GetCurrentContext());
  ESP_LOGI(TAG, "Recommended topic: %s", RecommendTopic());
  ESP_LOGI(TAG, "====================================");
}

int DecisionEngine::GetCurrentHour() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return timeinfo.tm_hour;
}

int64_t DecisionEngine::GetSecondsSinceLastInteraction() {
  if (!profile_) return 0;
  
  int64_t now_ms = esp_timer_get_time() / 1000;
  int64_t last_ms = profile_->GetData().last_update_ms;
  
  return (now_ms - last_ms) / 1000;
}

float DecisionEngine::RandomFloat() {
  uint32_t rand_val = esp_random();
  return (float)rand_val / (float)UINT32_MAX;
}

}  // namespace xiaozhi























