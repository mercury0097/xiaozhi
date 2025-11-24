#pragma once

#include <cstdint>
#include <string>

namespace xiaozhi {

// ============================================================================
// 情绪记忆类 - 长期情绪追踪和累积
// ============================================================================

struct EmotionalMemoryData {
  // 时间统计
  int64_t last_play_timestamp_ms;       // 最后玩耍时间
  int64_t last_feed_timestamp_ms;       // 最后喂食时间
  int64_t last_interaction_timestamp_ms; // 最后互动时间
  
  // 情绪趋势（滑动窗口，7天）
  float happiness_trend;                // 快乐趋势（-1.0 到 1.0）
  float energy_trend;                   // 精力趋势（-1.0 到 1.0）
  
  // 累积情绪
  int loneliness_level;                 // 孤独感（0-100）
  int excitement_level;                 // 兴奋度（0-100）
  int trust_level;                      // 信任度（0-100，长期累积）
  
  // 互动历史
  uint16_t total_play_count;            // 总玩耍次数
  uint16_t total_feed_count;            // 总喂食次数
  uint16_t total_hug_count;             // 总拥抱次数
  
  // 计算辅助方法
  int GetDaysSinceLastPlay(int64_t now_ms) const;
  int GetDaysSinceLastFeed(int64_t now_ms) const;
  int GetDaysSinceLastInteraction(int64_t now_ms) const;
  
  bool IsLonely() const { return loneliness_level > 60; }
  bool IsExcited() const { return excitement_level > 70; }
  bool IsTrusting() const { return trust_level > 70; }
};

// ============================================================================
// 情绪记忆管理器
// ============================================================================

class EmotionalMemory {
 public:
  static EmotionalMemory& GetInstance();

  // 初始化（从 NVS 加载）
  bool Initialize();

  // ========== 事件记录 ==========
  
  // 记录玩耍
  void RecordPlay();
  
  // 记录喂食
  void RecordFeed();
  
  // 记录拥抱
  void RecordHug();
  
  // 记录一般互动
  void RecordInteraction();
  
  // 记录正面事件（增加快乐）
  void RecordPositiveEvent(int happiness_delta = 10);
  
  // 记录负面事件（减少快乐）
  void RecordNegativeEvent(int happiness_delta = -10);

  // ========== 情绪更新 ==========
  
  // 每分钟更新（由定时器调用）
  void Update();
  
  // 更新孤独感（基于最后互动时间）
  void UpdateLoneliness();
  
  // 更新快乐趋势（基于近期事件）
  void UpdateHappinessTrend();
  
  // 更新信任度（长期累积）
  void UpdateTrust();

  // ========== 查询接口 ==========
  
  // 获取完整数据
  const EmotionalMemoryData& GetData() const { return data_; }
  
  // 获取孤独感
  int GetLonelinessLevel() const { return data_.loneliness_level; }
  
  // 获取兴奋度
  int GetExcitementLevel() const { return data_.excitement_level; }
  
  // 获取信任度
  int GetTrustLevel() const { return data_.trust_level; }
  
  // 获取快乐趋势
  float GetHappinessTrend() const { return data_.happiness_trend; }
  
  // 是否孤独
  bool IsLonely() const { return data_.IsLonely(); }
  
  // 是否兴奋
  bool IsExcited() const { return data_.IsExcited(); }
  
  // 是否信任主人
  bool IsTrusting() const { return data_.IsTrusting(); }

  // ========== 响应生成 ==========
  
  /**
   * @brief 获取长期情绪反应
   * @return 反应文案（如果没有特殊情绪则返回空字符串）
   */
  std::string GetLongtermResponse();
  
  /**
   * @brief 获取思念文案（长时间未互动）
   * @return 思念文案
   */
  std::string GetMissYouMessage();
  
  /**
   * @brief 获取兴奋文案（刚互动完）
   * @return 兴奋文案
   */
  std::string GetExcitedMessage();

  // ========== 持久化（NVS） ==========
  
  // 保存到 NVS
  bool SaveToNVS();
  
  // 从 NVS 加载
  bool LoadFromNVS();
  
  // 重置所有数据
  void Reset();

  // ========== 调试 ==========
  
  // 打印情绪状态
  void PrintStatus() const;

 private:
  EmotionalMemory() = default;
  ~EmotionalMemory() = default;
  EmotionalMemory(const EmotionalMemory&) = delete;
  EmotionalMemory& operator=(const EmotionalMemory&) = delete;

  // 初始化默认值
  void InitializeDefaults();
  
  // 辅助函数
  int64_t GetCurrentTimeMs() const;
  int Clamp(int value, int min = 0, int max = 100) const;
  float ClampF(float value, float min = -1.0f, float max = 1.0f) const;

  // 数据
  EmotionalMemoryData data_;
  
  // 配置
  uint32_t auto_save_interval_s_ = 300;  // 默认5分钟自动保存
  int64_t last_save_ms_ = 0;
  bool needs_save_ = false;
};

}  // namespace xiaozhi


