#pragma once

#include "user_profile.h"
#include <cstdint>

namespace xiaozhi {

// ============================================================================
// 决策引擎类
// ============================================================================

class DecisionEngine {
 public:
  static DecisionEngine& GetInstance();

  // 初始化（依赖UserProfile）
  bool Initialize();

  // ========== 概率决策 ==========
  
  // 获取主动问候概率（0.0 ~ 1.0）
  // 基于互动频率、时段、用户反馈
  float GetProactiveProbability();
  
  // 获取宠物提醒间隔（分钟）
  // 基于用户养宠习惯
  int GetPetReminderInterval();
  
  // 获取表情活跃度（0-100）
  // 基于用户正面反馈
  int GetEmotionLiveness();

  // ========== 情境感知 ==========
  
  // 获取当前情境
  enum Context {
    CONTEXT_MORNING,      // 早上 6-9点
    CONTEXT_WORK_TIME,    // 工作时间 9-18点
    CONTEXT_EVENING,      // 晚上 18-22点
    CONTEXT_NIGHT,        // 深夜 22-6点
    CONTEXT_LOW_BATTERY,  // 低电量
    CONTEXT_LONG_IDLE,    // 长时间未互动
  };
  Context GetCurrentContext();
  
  // 根据情境获取问候语
  const char* GetGreetingByContext(Context ctx);
  
  // ========== 个性化推荐 ==========
  
  // 推荐话题（基于历史偏好）
  const char* RecommendTopic();
  
  // 是否应该主动提醒宠物（基于时间和习惯）
  bool ShouldRemindPet();
  
  // 是否应该主动问候（基于概率）
  bool ShouldGreetProactively();

  // ========== 辅助功能 ==========
  
  // 打印当前决策参数（调试用）
  void PrintDecisionParams();

 private:
  DecisionEngine() = default;
  ~DecisionEngine() = default;
  DecisionEngine(const DecisionEngine&) = delete;
  DecisionEngine& operator=(const DecisionEngine&) = delete;

  // 获取当前小时（0-23）
  int GetCurrentHour();
  
  // 获取最后互动距今时间（秒）
  int64_t GetSecondsSinceLastInteraction();
  
  // 随机数生成（0.0 ~ 1.0）
  float RandomFloat();

  UserProfile* profile_ = nullptr;
  int64_t last_pet_reminder_ms_ = 0;
  int64_t last_proactive_greeting_ms_ = 0;
};

}  // namespace xiaozhi























