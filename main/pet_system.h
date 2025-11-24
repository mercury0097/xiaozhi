#ifndef _PET_SYSTEM_H_
#define _PET_SYSTEM_H_

#include <cstdint>
#include <string>
#include <map>
#include <functional>
#include <esp_timer.h>

/**
 * @brief 电子宠物系统 - 轻量级实现
 * 
 * 特性：
 * - 三维状态：心情(mood)、饱腹(satiety)、清洁(cleanliness)
 * - 自动衰减：久未互动会降低状态
 * - 每日任务：喂食、洗澡、玩耍、聊天
 * - 表情联动：自动根据状态切换表情
 * - NVS 持久化：断电不丢失
 * - CPU 友好：1分钟 tick，简单计算
 */

class PetSystem {
public:
    // 宠物类型配置
    struct PetType {
        std::string name;           // 名称（中文）
        std::string name_en;        // 英文名称
        std::string emoji;          // 表情符号
        std::string category;       // 分类：domestic(家养)/wild(野生)/exotic(奇异)
        float hunger_rate;          // 饥饿速度倍率（1.0=正常）
        float clean_rate;           // 脏污速度倍率（1.0=正常）
        float mood_decay_rate;      // 心情衰减倍率（1.0=正常）
        std::string personality;    // 性格描述
        std::string special_trait;  // 特殊能力
        bool is_available;          // 是否可用（预留）
    };

    // 宠物状态结构
    struct State {
        std::string petType = "cat";    // 宠物类型（默认猫咪）
        int mood = 70;          // 心情 0-100
        int satiety = 70;       // 饱腹度 0-100
        int cleanliness = 70;   // 清洁度 0-100
        int active = 50;        // 活跃度 0-100
        int level = 1;          // 等级 1-10
        int64_t lastUpdateMs = 0;       // 最后更新时间
        int64_t lastInteractionMs = 0;  // 最后交互时间
        uint32_t dailyDoneMask = 0;     // 每日任务完成标记 (bit0:feed, bit1:clean, bit2:play, bit3:chat5)
        int loginStreak = 0;            // 连续登录天数
        int64_t lastResetDayMs = 0;     // 最后重置日期
    };

    // 每日任务类型
    enum DailyTask {
        TASK_FEED = 0,      // 喂食
        TASK_CLEAN = 1,     // 洗澡
        TASK_PLAY = 2,      // 玩耍
        TASK_CHAT = 3,      // 聊天
    };

    static PetSystem& GetInstance() {
        static PetSystem instance;
        return instance;
    }

    // 删除拷贝构造和赋值
    PetSystem(const PetSystem&) = delete;
    PetSystem& operator=(const PetSystem&) = delete;

    /**
     * @brief 启动宠物系统
     */
    void Start();

    /**
     * @brief 停止宠物系统
     */
    void Stop();

    /**
     * @brief 获取当前状态
     */
    const State& GetState() const { return state_; }

    /**
     * @brief 选择宠物类型
     * @param type_name 宠物类型名称（如 "cat", "dog", "lion" 等）
     * @return true 成功，false 失败（类型不存在）
     */
    bool SelectPetType(const std::string& type_name);

    /**
     * @brief 获取当前宠物类型信息
     */
    const PetType* GetCurrentPetType() const;

    /**
     * @brief 列出所有可用的宠物类型
     * @return JSON 格式的宠物类型列表
     */
    std::string ListPetTypes() const;

    /**
     * @brief 获取指定宠物类型的详细信息
     */
    std::string GetPetTypeInfo(const std::string& type_name) const;

    /**
     * @brief 喂食
     * @param amount 食物量 (1-10)
     * @return true 成功，false 失败（冷却中或已满）
     */
    bool Feed(int amount = 5);

    /**
     * @brief 清洁/洗澡
     * @return true 成功，false 失败（冷却中或已满）
     */
    bool Clean();

    /**
     * @brief 玩耍
     * @param kind 玩耍类型（预留，可选："dance", "ball", "chat"）
     * @return true 成功，false 失败（冷却中）
     */
    bool Play(const std::string& kind = "");

    /**
     * @brief 拥抱/互动
     * @return true 成功
     */
    bool Hug();

    /**
     * @brief 记录聊天（用于每日任务统计）
     */
    void RecordChat();

    /**
     * @brief 获取综合状态（0-100）
     */
    int GetOverallState() const;

    /**
     * @brief 获取建议的表情
     * @return 表情名称：happy, sad, neutral, thinking, embarrassed等
     */
    std::string GetRecommendedEmotion() const;

    /**
     * @brief 获取状态描述
     * @return JSON 格式的状态描述
     */
    std::string GetStatusDescription() const;

    /**
     * @brief 获取建议事项
     * @return 字符串列表，如"该喂食了"、"需要洗澡"
     */
    std::string GetSuggestions() const;

    /**
     * @brief 重置每日任务（测试用，正常由定时器触发）
     */
    void ResetDaily();

    /**
     * @brief 设置调试模式（用于测试，可直接修改状态）
     * @param mood 心情
     * @param satiety 饱腹度
     * @param cleanliness 清洁度
     */
    void DebugSet(int mood, int satiety, int cleanliness);

    /**
     * @brief 设置状态警告回调（当宠物状态需要关注时触发）
     * @param callback 回调函数，参数为警告消息
     */
    void SetWarningCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief 手动触发警告（用于测试或对话后检查）
     * @param warning 警告消息
     */
    void TriggerWarning(const std::string& warning);

    /**
     * @brief 启用/禁用自动播报
     * @param enable true=启用自动播报，false=禁用
     * @param interval_min 播报间隔（分钟），默认3分钟
     */
    void EnableAutoAnnouncement(bool enable, int interval_min = 3);
    
    /**
     * @brief 检查是否需要发出警告
     * @return 警告消息，如果不需要警告则返回空字符串
     */
    std::string CheckWarning();

private:
    PetSystem();
    ~PetSystem();

    // 定时器回调（1分钟tick）
    static void TimerCallback(void* arg);
    void OnTick();
    
    // 状态检查和警告
    void CheckAndNotifyWarnings();

    // 加载和保存状态
    void LoadState();
    void SaveState();

    // 状态更新
    void UpdateDecay();      // 衰减计算
    void CheckDailyReset();  // 检查每日重置
    void ClampState();       // 限制状态范围

    // 🧠 自适应学习功能
    float GetAdaptiveDecayRate() const;              // 获取自适应衰减速率
    std::string GetProbabilisticResponse() const;    // 获取概率性反应
    bool ShouldSuppressWarningByContext() const;     // 根据情境判断是否抑制警告
    float GetEmotionalMomentum() const;              // 获取情绪动量

    // 工具函数
    int Clamp(int value, int min = 0, int max = 100) const;
    bool CheckCooldown(int64_t& lastActionMs, int cooldownSeconds) const;
    int64_t GetCurrentTimeMs() const;
    int GetDaysSince1970(int64_t timestampMs) const;
    
    // 🛡️ 安全的消息构建（防止字符串拼接异常）
    std::string BuildWarningMessage(const PetType* pet_type, const std::string& base_message);

    State state_;
    esp_timer_handle_t timer_handle_ = nullptr;
    bool started_ = false;

    // 宠物类型数据库
    std::map<std::string, PetType> pet_types_;
    void InitializePetTypes();  // 初始化宠物类型数据

    // 冷却时间戳
    int64_t lastFeedMs_ = 0;
    int64_t lastCleanMs_ = 0;
    int64_t lastPlayMs_ = 0;
    int64_t lastHugMs_ = 0;
    int chatCountToday_ = 0;
    
    // 警告回调和冷却
    std::function<void(const std::string&)> warning_callback_;
    int64_t lastWarningMs_ = 0;
    
    // 自动播报控制
    bool auto_announcement_enabled_ = true;   // 默认启用
    int auto_announcement_interval_min_ = 3;  // 默认3分钟
    int64_t last_announcement_ms_ = 0;        // 上次播报时间
    std::string lastWarningType_;  // 记录上次警告类型，避免重复

    // 配置参数
    static constexpr int FEED_COOLDOWN_SEC = 60;       // 喂食冷却1分钟
    static constexpr int CLEAN_COOLDOWN_SEC = 120;     // 洗澡冷却2分钟
    static constexpr int PLAY_COOLDOWN_SEC = 60;       // 玩耍冷却1分钟
    static constexpr int HUG_COOLDOWN_SEC = 30;        // 拥抱冷却30秒
    
    static constexpr int DECAY_SATIETY_PER_MIN = 1;    // 每分钟饱腹度衰减
    static constexpr int DECAY_CLEAN_PER_MIN = 1;      // 每分钟清洁度衰减
    static constexpr int DECAY_MOOD_PER_MIN = 1;       // 每分钟心情衰减（无互动时）
    
    static constexpr int NO_INTERACTION_MIN = 10;      // 10分钟无互动判定
    
    // 警告阈值
    static constexpr int WARNING_SATIETY_THRESHOLD = 30;     // 饱腹度低于30%警告
    static constexpr int WARNING_CLEAN_THRESHOLD = 25;       // 清洁度低于25%警告
    static constexpr int WARNING_MOOD_THRESHOLD = 35;        // 心情低于35%警告
    static constexpr int WARNING_COOLDOWN_MIN = 15;          // 15分钟警告冷却
};

#endif // _PET_SYSTEM_H_

