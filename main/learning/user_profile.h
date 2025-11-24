#pragma once

#include <cstdint>
#include <string>

namespace xiaozhi {

// ============================================================================
// 用户画像数据结构
// ============================================================================

struct UserProfileData {
  // 互动统计（7天滑动窗口）
  uint32_t interaction_count_7d;      // 7天内对话次数
  uint32_t avg_session_duration_s;    // 平均会话时长（秒）
  
  // 时间偏好（24小时活跃度分布）
  uint8_t active_hours[24];           // 每小时活跃度 0-255
  
  // 话题偏好（计数）
  uint16_t topic_weather;             // 天气查询次数
  uint16_t topic_story;               // 听故事次数
  uint16_t topic_pet;                 // 照顾宠物次数
  uint16_t topic_smart_home;          // 控制智能家居次数
  uint16_t topic_chat;                // 闲聊次数
  uint16_t topic_other;               // 其他话题
  
  // 情感倾向
  uint16_t positive_feedback_count;   // 正面反馈次数
  uint16_t negative_feedback_count;   // 负面反馈次数
  
  // 时间戳
  int64_t last_update_ms;             // 最后更新时间（毫秒时间戳）
  int64_t created_ms;                 // 创建时间
  
  // 辅助计算
  uint8_t GetPositiveRatio() const {
    uint32_t total = positive_feedback_count + negative_feedback_count;
    if (total == 0) return 50;  // 默认50%
    return (positive_feedback_count * 100) / total;
  }
  
  uint8_t GetMostActiveHour() const {
    uint8_t max_hour = 0;
    uint8_t max_value = 0;
    for (int i = 0; i < 24; i++) {
      if (active_hours[i] > max_value) {
        max_value = active_hours[i];
        max_hour = i;
      }
    }
    return max_hour;
  }
};

// ============================================================================
// 用户画像类
// ============================================================================

class UserProfile {
 public:
  static UserProfile& GetInstance();

  // 初始化（从SPIFFS加载）
  bool Initialize();

  // ========== 数据记录 ==========
  
  // 记录一次互动
  void RecordInteraction(const char* topic, uint32_t duration_ms);
  
  // 记录用户反馈
  void RecordFeedback(bool is_positive);
  
  // 更新时间活跃度（自动调用，每次对话时）
  void UpdateTimeActivity();

  // ========== 数据查询 ==========
  
  // 获取完整数据
  const UserProfileData& GetData() const { return data_; }
  
  // 获取7天互动次数
  uint32_t GetInteractionCount7d() const { return data_.interaction_count_7d; }
  
  // 获取最活跃时段
  uint8_t GetMostActiveHour() const { return data_.GetMostActiveHour(); }
  
  // 获取正面反馈比例（0-100）
  uint8_t GetPositiveRatio() const { return data_.GetPositiveRatio(); }
  
  // 获取话题偏好
  const char* GetFavoriteTopic() const;

  // ========== 持久化（NVS） ==========
  
  // 保存到 NVS
  bool SaveToNVS();
  
  // 从 NVS 加载
  bool LoadFromNVS();
  
  // 重置所有数据
  void Reset();

  // ========== 辅助功能 ==========
  
  // 打印统计信息（调试用）
  void PrintStats() const;
  
  // 设置自动保存间隔（秒）
  void SetAutoSaveInterval(uint32_t seconds);
  
  // 手动触发自动保存检查
  void CheckAutoSave();

 private:
  UserProfile() = default;
  ~UserProfile() = default;
  UserProfile(const UserProfile&) = delete;
  UserProfile& operator=(const UserProfile&) = delete;

  // 初始化数据为默认值
  void InitializeDefaults();
  
  // 解析话题类型
  void IncrementTopicCount(const char* topic);

  // 数据
  UserProfileData data_;
  
  // 配置
  uint32_t auto_save_interval_s_ = 600;  // 默认10分钟自动保存
  int64_t last_save_ms_ = 0;
  bool needs_save_ = false;
};

}  // namespace xiaozhi
