#pragma once

#include "user_profile.h"
#include "decision_engine.h"
#include <cstdint>
#include <string>

namespace xiaozhi {

// ============================================================================
// 自适应行为类 - 整合用户画像和决策引擎，提供统一的自适应接口
// ============================================================================

class AdaptiveBehavior {
 public:
  static AdaptiveBehavior& GetInstance();

  // 初始化（依赖 UserProfile 和 DecisionEngine）
  bool Initialize();

  // ========== 🐾 宠物系统自适应 ==========
  
  /**
   * @brief 获取宠物衰减速率倍率
   * @return 衰减速率（0.5 ~ 1.5）
   *         高频用户：1.2-1.5x（更需要照顾）
   *         中频用户：1.0x（正常）
   *         低频用户：0.5-0.8x（减慢衰减）
   */
  float GetPetDecayRate();
  
  /**
   * @brief 获取宠物警告阈值调整
   * @return 阈值偏移量（-10 ~ +10）
   *         高频用户：+5（更早警告）
   *         低频用户：-5（更晚警告）
   */
  int GetPetWarningThresholdOffset();
  
  /**
   * @brief 判断是否应该抑制宠物警告（情境感知）
   * @return true = 抑制（夜间/工作时/用户不活跃）
   */
  bool ShouldSuppressPetWarning();
  
  /**
   * @brief 获取概率性宠物反应文案
   * @param mood 宠物心情值（0-100）
   * @param warning_type 警告类型（"hunger" / "clean" / "mood"）
   * @return 反应文案
   */
  std::string GetProbabilisticPetResponse(int mood, const char* warning_type);

  // ========== 🎭 表情系统自适应 ==========
  
  /**
   * @brief 获取表情活跃度
   * @return 活跃度（30-100）
   *         高反馈用户：70-100（表情丰富）
   *         低反馈用户：30-50（表情保守）
   */
  int GetEmotionLiveness();
  
  /**
   * @brief 根据宠物心情获取推荐表情
   * @param mood 心情值（0-100）
   * @return 表情名称（"happy" / "sad" / "neutral" 等）
   */
  const char* GetEmotionByMood(int mood);
  
  /**
   * @brief 判断是否应该使用动画效果
   * @return true = 使用动画（用户喜欢丰富表情）
   */
  bool ShouldUseAnimatedEmotion();

  // ========== 💬 对话系统自适应 ==========
  
  /**
   * @brief 获取自适应问候语
   * @return 问候语（根据时段、用户频率）
   */
  const char* GetGreeting();
  
  /**
   * @brief 获取推荐话题
   * @return 话题建议（基于历史偏好）
   */
  const char* GetRecommendedTopic();
  
  /**
   * @brief 判断是否应该主动问候
   * @return true = 应该主动问候（根据概率决定）
   */
  bool ShouldGreetProactively();
  
  /**
   * @brief 获取宠物提醒间隔（分钟）
   * @return 间隔时间（30-120 分钟）
   */
  int GetPetReminderInterval();

  // ========== ⚙️ 通用工具 ==========
  
  /**
   * @brief 生成随机浮点数
   * @return 0.0 ~ 1.0
   */
  float RandomFloat();
  
  /**
   * @brief 概率判定（掷骰子）
   * @param probability 概率（0.0 ~ 1.0）
   * @return true = 成功
   */
  bool RollProbability(float probability);
  
  /**
   * @brief 打印当前自适应参数（调试用）
   */
  void PrintAdaptiveParams();

  // ========== 📊 统计查询 ==========
  
  /**
   * @brief 获取用户互动频率等级
   * @return 0 = 低频，1 = 中频，2 = 高频
   */
  int GetUserFrequencyLevel();
  
  /**
   * @brief 获取当前时段的活跃概率
   * @return 概率（0-100）
   */
  int GetCurrentHourActivity();

 private:
  AdaptiveBehavior() = default;
  ~AdaptiveBehavior() = default;
  AdaptiveBehavior(const AdaptiveBehavior&) = delete;
  AdaptiveBehavior& operator=(const AdaptiveBehavior&) = delete;

  // 依赖组件
  UserProfile* profile_ = nullptr;
  DecisionEngine* decision_ = nullptr;
  
  // 辅助函数
  int GetCurrentHour();
  bool IsNightTime();
  bool IsWorkTime();
};

}  // namespace xiaozhi


