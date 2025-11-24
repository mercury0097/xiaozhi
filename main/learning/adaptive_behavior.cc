#include "adaptive_behavior.h"
#include <esp_log.h>
#include <esp_random.h>
#include <ctime>
#include <cstring>

namespace xiaozhi {

static const char* TAG = "AdaptiveBehavior";

// ============================================================================
// 单例和初始化
// ============================================================================

AdaptiveBehavior& AdaptiveBehavior::GetInstance() {
  static AdaptiveBehavior instance;
  return instance;
}

bool AdaptiveBehavior::Initialize() {
  profile_ = &UserProfile::GetInstance();
  decision_ = &DecisionEngine::GetInstance();
  
  ESP_LOGI(TAG, "Adaptive behavior system initialized");
  ESP_LOGI(TAG, "  User frequency level: %d", GetUserFrequencyLevel());
  ESP_LOGI(TAG, "  Pet decay rate: %.2f", GetPetDecayRate());
  
  return true;
}

// ============================================================================
// 🐾 宠物系统自适应
// ============================================================================

float AdaptiveBehavior::GetPetDecayRate() {
  if (!profile_) return 1.0f;
  
  uint32_t interactions = profile_->GetInteractionCount7d();
  
  // 🎯 自适应衰减速率
  if (interactions > 50) {
    // 高频用户（7天>50次）→ 衰减加快 30%
    return 1.3f;
  } else if (interactions > 30) {
    // 中高频用户 → 衰减加快 20%
    return 1.2f;
  } else if (interactions > 15) {
    // 中频用户 → 正常衰减
    return 1.0f;
  } else if (interactions > 5) {
    // 低频用户 → 衰减减慢 20%
    return 0.8f;
  } else {
    // 极低频用户 → 衰减减慢 40%
    return 0.6f;
  }
}

int AdaptiveBehavior::GetPetWarningThresholdOffset() {
  if (!profile_) return 0;
  
  uint32_t interactions = profile_->GetInteractionCount7d();
  
  // 高频用户：提前警告（阈值+5）
  if (interactions > 40) {
    return +5;
  }
  // 低频用户：延后警告（阈值-5）
  else if (interactions < 10) {
    return -5;
  }
  
  return 0;
}

bool AdaptiveBehavior::ShouldSuppressPetWarning() {
  if (!profile_ || !decision_) return false;
  
  // 🌙 夜间时段（23:00 - 7:00）
  if (IsNightTime()) {
    ESP_LOGD(TAG, "Suppress warning: night time");
    return true;
  }
  
  // 🏢 工作时间 + 用户不活跃
  if (IsWorkTime()) {
    int hour = GetCurrentHour();
    const auto& data = profile_->GetData();
    
    // 如果用户在这个时段活跃度很低（<5次），判断为不适合打扰
    if (data.active_hours[hour] < 5) {
      ESP_LOGD(TAG, "Suppress warning: work time + low activity");
      return true;
    }
  }
  
  return false;
}

std::string AdaptiveBehavior::GetProbabilisticPetResponse(int mood, const char* warning_type) {
  if (!warning_type) return "";
  
  float rand = RandomFloat();
  
  // 🍖 饥饿警告
  if (strcmp(warning_type, "hunger") == 0) {
    if (mood > 70) {
      // 心情好时，撒娇 70%，活泼 30%
      if (rand < 0.7f) {
        return "主人～肚子好饿呀，给我喂点好吃的嘛～";
      } else {
        return "主人！我饿了！快给我吃的！";
      }
    } else if (mood > 40) {
      // 心情一般，平静陈述
      return "主人，我有点饿了，要不要喂我一下？";
    } else {
      // 心情差时，低落 60%，傲娇 40%
      if (rand < 0.6f) {
        return "主人...人家好饿...都不理我...";
      } else {
        return "哼，都不给我吃的...";
      }
    }
  }
  
  // 🛁 清洁警告
  else if (strcmp(warning_type, "clean") == 0) {
    if (mood > 70) {
      if (rand < 0.7f) {
        return "主人～人家脏脏的啦，帮我洗澡嘛～";
      } else {
        return "主人！我需要洗香香了！";
      }
    } else if (mood > 40) {
      return "主人，我需要清洁一下了，不然会不舒服的。";
    } else {
      if (rand < 0.6f) {
        return "主人...人家身上好难受...";
      } else {
        return "哼，都不帮我洗澡...";
      }
    }
  }
  
  // 💔 心情警告
  else if (strcmp(warning_type, "mood") == 0) {
    if (rand < 0.5f) {
      return "主人...人家心情不太好，陪陪我嘛...";
    } else {
      return "主人，好久没陪我玩了，人家好无聊...";
    }
  }
  
  // 默认
  return "主人，注意一下我嘛～";
}

// ============================================================================
// 🎭 表情系统自适应
// ============================================================================

int AdaptiveBehavior::GetEmotionLiveness() {
  if (!decision_) return 50;
  return decision_->GetEmotionLiveness();  // 30-100
}

const char* AdaptiveBehavior::GetEmotionByMood(int mood) {
  if (mood > 80) {
    return "happy";
  } else if (mood > 60) {
    return "neutral";
  } else if (mood > 40) {
    return "thinking";
  } else if (mood > 20) {
    return "sad";
  } else {
    return "embarrassed";
  }
}

bool AdaptiveBehavior::ShouldUseAnimatedEmotion() {
  // 固定返回 true，不再基于活跃度自适应
  return true;
}

// ============================================================================
// 💬 对话系统自适应
// ============================================================================

const char* AdaptiveBehavior::GetGreeting() {
  if (!decision_) return "你好！";
  
  auto ctx = decision_->GetCurrentContext();
  return decision_->GetGreetingByContext(ctx);
}

const char* AdaptiveBehavior::GetRecommendedTopic() {
  if (!decision_) return "要不要聊聊天？";
  return decision_->RecommendTopic();
}

bool AdaptiveBehavior::ShouldGreetProactively() {
  if (!decision_) return false;
  return decision_->ShouldGreetProactively();
}

int AdaptiveBehavior::GetPetReminderInterval() {
  if (!decision_) return 90;  // 默认90分钟
  return decision_->GetPetReminderInterval();
}

// ============================================================================
// ⚙️ 通用工具
// ============================================================================

float AdaptiveBehavior::RandomFloat() {
  uint32_t rand_val = esp_random();
  return (float)rand_val / (float)UINT32_MAX;
}

bool AdaptiveBehavior::RollProbability(float probability) {
  return RandomFloat() < probability;
}

void AdaptiveBehavior::PrintAdaptiveParams() {
  ESP_LOGI(TAG, "========== Adaptive Parameters ==========");
  ESP_LOGI(TAG, "User frequency: %d (0=low, 1=mid, 2=high)", GetUserFrequencyLevel());
  ESP_LOGI(TAG, "Pet decay rate: %.2f", GetPetDecayRate());
  ESP_LOGI(TAG, "Pet warning offset: %+d", GetPetWarningThresholdOffset());
  ESP_LOGI(TAG, "Pet reminder interval: %d min", GetPetReminderInterval());
  ESP_LOGI(TAG, "Current hour activity: %d%%", GetCurrentHourActivity());
  ESP_LOGI(TAG, "Night time: %s", IsNightTime() ? "yes" : "no");
  ESP_LOGI(TAG, "Work time: %s", IsWorkTime() ? "yes" : "no");
  ESP_LOGI(TAG, "========================================");
}

// ============================================================================
// 📊 统计查询
// ============================================================================

int AdaptiveBehavior::GetUserFrequencyLevel() {
  if (!profile_) return 1;
  
  uint32_t interactions = profile_->GetInteractionCount7d();
  
  if (interactions > 30) {
    return 2;  // 高频
  } else if (interactions > 10) {
    return 1;  // 中频
  } else {
    return 0;  // 低频
  }
}

int AdaptiveBehavior::GetCurrentHourActivity() {
  if (!profile_) return 50;
  
  int hour = GetCurrentHour();
  const auto& data = profile_->GetData();
  
  // 活跃度 0-255 → 转换为百分比
  return (data.active_hours[hour] * 100) / 255;
}

// ============================================================================
// 🔧 辅助函数
// ============================================================================

int AdaptiveBehavior::GetCurrentHour() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return timeinfo.tm_hour;
}

bool AdaptiveBehavior::IsNightTime() {
  int hour = GetCurrentHour();
  return (hour >= 23 || hour < 7);
}

bool AdaptiveBehavior::IsWorkTime() {
  int hour = GetCurrentHour();
  return (hour >= 9 && hour < 18);
}

}  // namespace xiaozhi

