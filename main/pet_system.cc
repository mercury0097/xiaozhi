#include "pet_system.h"
#include "settings.h"
#include "board.h"
#include "display/display.h"
#include "core/event_bus.h"
#include "learning/adaptive_behavior.h"
#include "learning/emotional_memory.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <cmath>
#include <algorithm>
#include <cJSON.h>

#define TAG "Pet"

PetSystem::PetSystem() {
    InitializePetTypes();
}

PetSystem::~PetSystem() {
    Stop();
}

void PetSystem::InitializePetTypes() {
    // 家养宠物（domestic）
    pet_types_["cat"] = {
        "猫咪", "Cat", "🐱", "domestic",
        0.8f, 1.2f, 0.9f,
        "独立、优雅、偶尔高冷",
        "夜间活跃度+20%",
        true
    };
    
    pet_types_["dog"] = {
        "小狗", "Dog", "🐶", "domestic",
        1.2f, 1.5f, 1.3f,
        "忠诚、活泼、需要陪伴",
        "每次互动心情+2额外加成",
        true
    };
    
    pet_types_["rabbit"] = {
        "兔子", "Rabbit", "🐰", "domestic",
        1.1f, 0.8f, 1.0f,
        "温顺、胆小、喜欢安静",
        "清洁度>80时心情+5",
        true
    };
    
    pet_types_["hamster"] = {
        "仓鼠", "Hamster", "🐹", "domestic",
        0.9f, 1.0f, 0.8f,
        "小巧、可爱、储存狂",
        "喂食冷却-30秒",
        true
    };
    
    pet_types_["parrot"] = {
        "鹦鹉", "Parrot", "🦜", "domestic",
        0.7f, 0.9f, 1.1f,
        "聪明、话痨、喜欢学舌",
        "聊天任务完成速度x2",
        true
    };
    
    // 野生动物（wild）
    pet_types_["lion"] = {
        "狮子", "Lion", "🦁", "wild",
        1.5f, 0.7f, 1.2f,
        "威武、霸气、百兽之王",
        "饱腹度<30时攻击性+50%",
        true
    };
    
    pet_types_["tiger"] = {
        "老虎", "Tiger", "🐯", "wild",
        1.6f, 0.8f, 1.3f,
        "凶猛、强壮、独居",
        "每日首次互动心情+10",
        true
    };
    
    pet_types_["panda"] = {
        "熊猫", "Panda", "🐼", "wild",
        2.0f, 0.6f, 0.7f,
        "慵懒、可爱、吃货",
        "喂食效果x1.5",
        true
    };
    
    pet_types_["bear"] = {
        "熊", "Bear", "🐻", "wild",
        1.8f, 0.9f, 1.1f,
        "强壮、贪吃、冬眠",
        "饱腹度>80时活跃度+10",
        true
    };
    
    pet_types_["wolf"] = {
        "狼", "Wolf", "🐺", "wild",
        1.4f, 0.8f, 1.4f,
        "忠诚、团结、野性",
        "连续登录奖励x2",
        true
    };
    
    pet_types_["fox"] = {
        "狐狸", "Fox", "🦊", "wild",
        1.0f, 1.1f, 1.0f,
        "聪明、狡猾、灵活",
        "玩耍效果+50%",
        true
    };
    
    // 奇异动物（exotic）
    pet_types_["penguin"] = {
        "企鹅", "Penguin", "🐧", "exotic",
        1.1f, 0.5f, 0.9f,
        "憨厚、怕热、游泳健将",
        "清洁效果x1.8（爱洗澡）",
        true
    };
    
    pet_types_["rhino"] = {
        "犀牛", "Rhino", "🦏", "exotic",
        1.3f, 2.0f, 0.8f,
        "笨重、脾气暴躁、皮厚",
        "清洁度衰减x2但心情稳定",
        true
    };
    
    pet_types_["elephant"] = {
        "大象", "Elephant", "🐘", "exotic",
        2.5f, 1.8f, 0.6f,
        "温和、记性好、群居",
        "不会忘记互动，心情永不低于40",
        true
    };
    
    pet_types_["giraffe"] = {
        "长颈鹿", "Giraffe", "🦒", "exotic",
        1.2f, 1.0f, 0.8f,
        "温顺、高大、挑食",
        "饱腹度>60才会增加心情",
        true
    };
    
    pet_types_["koala"] = {
        "考拉", "Koala", "🐨", "exotic",
        0.6f, 0.7f, 0.5f,
        "懒惰、嗜睡、吃货",
        "所有衰减速度-40%",
        true
    };
    
    pet_types_["sloth"] = {
        "树懒", "Sloth", "🦥", "exotic",
        0.5f, 0.6f, 0.4f,
        "超级慵懒、行动缓慢",
        "所有状态衰减最慢！",
        true
    };
    
    pet_types_["dragon"] = {
        "龙", "Dragon", "🐉", "exotic",
        1.8f, 1.0f, 1.5f,
        "神秘、强大、传说生物",
        "等级提升速度x2",
        true
    };
    
    pet_types_["unicorn"] = {
        "独角兽", "Unicorn", "🦄", "exotic",
        0.8f, 0.5f, 0.6f,
        "纯洁、优雅、魔法生物",
        "心情永不低于50",
        true
    };
    
    ESP_LOGI(TAG, "✨ Initialized %d pet types", pet_types_.size());
}

void PetSystem::Start() {
    if (started_) {
        ESP_LOGW(TAG, "Pet system already started");
        return;
    }

    ESP_LOGI(TAG, "🐾 Starting pet system...");
    
    // 加载状态
    LoadState();
    
    // 检查每日重置
    CheckDailyReset();
    
    // 创建定时器（1分钟tick）
    esp_timer_create_args_t timer_args = {
        .callback = TimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "pet_timer",
        .skip_unhandled_events = true
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &timer_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create pet timer: %d", err);
        return;
    }
    
    // 启动定时器（60秒周期）
    err = esp_timer_start_periodic(timer_handle_, 60 * 1000000ULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start pet timer: %d", err);
        esp_timer_delete(timer_handle_);
        timer_handle_ = nullptr;
        return;
    }
    
    started_ = true;
    
    auto pet_type = GetCurrentPetType();
    ESP_LOGI(TAG, "✅ Pet system started successfully");
    ESP_LOGI(TAG, "   Pet: %s %s, Mood: %d, Satiety: %d, Clean: %d, Level: %d", 
             pet_type ? pet_type->emoji.c_str() : "?",
             pet_type ? pet_type->name.c_str() : "Unknown",
             state_.mood, state_.satiety, state_.cleanliness, state_.level);
    
    // 初始化显示当前宠物图标
    if (pet_type) {
        auto& board = Board::GetInstance();
        auto display = board.GetDisplay();
        if (display) {
            display->SetPetEmoji(pet_type->emoji.c_str());
            ESP_LOGI(TAG, "Initialized pet icon: %s", pet_type->emoji.c_str());
        }
    } else {
        ESP_LOGW(TAG, "⚠️  Pet type not found, using default 'cat'");
        state_.petType = "cat";
        SaveState();
    }
}

void PetSystem::Stop() {
    if (!started_) {
        return;
    }
    
    if (timer_handle_) {
        esp_timer_stop(timer_handle_);
        esp_timer_delete(timer_handle_);
        timer_handle_ = nullptr;
    }
    
    SaveState();
    started_ = false;
    
    ESP_LOGI(TAG, "🐾 Pet system stopped");
}

void PetSystem::TimerCallback(void* arg) {
    PetSystem* pet = static_cast<PetSystem*>(arg);
    pet->OnTick();
}

void PetSystem::OnTick() {
    int64_t now = GetCurrentTimeMs();
    
    // 更新衰减
    UpdateDecay();
    
    // 检查每日重置
    CheckDailyReset();
    
    // 检查是否需要发出警告
    CheckAndNotifyWarnings();
    
    // 每5分钟保存一次
    static int tick_count = 0;
    tick_count++;
    if (tick_count >= 5) {
        SaveState();
        tick_count = 0;
    }
    
    state_.lastUpdateMs = now;
    
    // 输出状态（调试用，每10分钟一次）
    static int log_count = 0;
    log_count++;
    if (log_count >= 10) {
        int overall = GetOverallState();
        ESP_LOGI(TAG, "📊 Status: Mood=%d, Satiety=%d, Clean=%d, Overall=%d", 
                 state_.mood, state_.satiety, state_.cleanliness, overall);
        log_count = 0;
    }
}

void PetSystem::UpdateDecay() {
    int64_t now = GetCurrentTimeMs();
    
    // 🛡️ 防止时钟回退导致的异常值
    if (now < state_.lastInteractionMs) {
        ESP_LOGW(TAG, "⚠️  Time went backwards! Resetting interaction time.");
        state_.lastInteractionMs = now;
    }
    
    int64_t sinceLastInteraction = (now - state_.lastInteractionMs) / 1000 / 60; // 分钟
    
    // 🧠 获取自适应衰减速率（基于用户互动频率）
    auto& adaptive = xiaozhi::AdaptiveBehavior::GetInstance();
    float adaptive_rate = adaptive.GetPetDecayRate();
    
    // 获取当前宠物类型的衰减倍率
    auto pet_type = GetCurrentPetType();
    float hunger_rate = pet_type ? pet_type->hunger_rate : 1.0f;
    float clean_rate = pet_type ? pet_type->clean_rate : 1.0f;
    float mood_rate = pet_type ? pet_type->mood_decay_rate : 1.0f;
    
    // 🎯 应用双重倍率：宠物类型 × 自适应速率
    // 高频用户：衰减更快（更需要照顾）
    // 低频用户：衰减更慢（减少打扰）
    float final_hunger_rate = hunger_rate * adaptive_rate;
    float final_clean_rate = clean_rate * adaptive_rate;
    float final_mood_rate = mood_rate * adaptive_rate;
    
    // 饱腹度衰减
    state_.satiety = Clamp(state_.satiety - static_cast<int>(DECAY_SATIETY_PER_MIN * final_hunger_rate));
    
    // 清洁度衰减
    state_.cleanliness = Clamp(state_.cleanliness - static_cast<int>(DECAY_CLEAN_PER_MIN * final_clean_rate));
    
    // 心情衰减（无互动时额外衰减）
    if (sinceLastInteraction > NO_INTERACTION_MIN) {
        state_.mood = Clamp(state_.mood - static_cast<int>(DECAY_MOOD_PER_MIN * 2 * final_mood_rate));
    } else {
        state_.mood = Clamp(state_.mood - static_cast<int>(DECAY_MOOD_PER_MIN * final_mood_rate));
    }
    
    ClampState();
    
    // 📊 自适应衰减日志（每次都显示，让用户看到学习效果）
    ESP_LOGI(TAG, "🔄 Decay: adaptive_rate=%.2fx (7d互动=%u次), mood=%d→%d, satiety=%d, clean=%d",
             adaptive_rate,
             xiaozhi::UserProfile::GetInstance().GetInteractionCount7d(),
             state_.mood + static_cast<int>(DECAY_MOOD_PER_MIN * final_mood_rate * (sinceLastInteraction > NO_INTERACTION_MIN ? 2 : 1)),
             state_.mood, state_.satiety, state_.cleanliness);
}

void PetSystem::CheckDailyReset() {
    int64_t now = GetCurrentTimeMs();
    int currentDay = GetDaysSince1970(now);
    int lastDay = GetDaysSince1970(state_.lastResetDayMs);
    
    if (currentDay > lastDay) {
        ESP_LOGI(TAG, "🌅 New day! Resetting daily tasks...");
        
        // 检查连续登录
        if (currentDay - lastDay == 1) {
            state_.loginStreak++;
            ESP_LOGI(TAG, "🔥 Login streak: %d days", state_.loginStreak);
            
            // 连续登录奖励
            if (state_.loginStreak >= 3) {
                state_.active = Clamp(state_.active + 5);
                ESP_LOGI(TAG, "🎁 Login bonus: +5 active");
            }
        } else if (currentDay - lastDay > 1) {
            state_.loginStreak = 1;
            ESP_LOGI(TAG, "Login streak reset");
        }
        
        state_.dailyDoneMask = 0;
        state_.lastResetDayMs = now;
        chatCountToday_ = 0;
        SaveState();
    }
}

void PetSystem::ClampState() {
    state_.mood = Clamp(state_.mood);
    state_.satiety = Clamp(state_.satiety);
    state_.cleanliness = Clamp(state_.cleanliness);
    state_.active = Clamp(state_.active);
    state_.level = Clamp(state_.level, 1, 10);
}

bool PetSystem::Feed(int amount) {
    if (!CheckCooldown(lastFeedMs_, FEED_COOLDOWN_SEC)) {
        ESP_LOGW(TAG, "Feed on cooldown");
        return false;
    }
    
    amount = Clamp(amount, 1, 10);
    
    if (state_.satiety >= 90) {
        ESP_LOGI(TAG, "🍔 Pet is full!");
        return false;
    }
    
    state_.satiety = Clamp(state_.satiety + amount * 2);
    state_.mood = Clamp(state_.mood + 3);
    
    // 溢出惩罚
    if (state_.satiety > 95) {
        state_.cleanliness = Clamp(state_.cleanliness - 2);
        ESP_LOGI(TAG, "😰 Overfed! Cleanliness -2");
    }
    
    // 完成每日任务
    if (!(state_.dailyDoneMask & (1 << TASK_FEED))) {
        state_.dailyDoneMask |= (1 << TASK_FEED);
        state_.active = Clamp(state_.active + 5);
        ESP_LOGI(TAG, "✅ Daily task: Feed completed! +5 active");
    }
    
    state_.lastInteractionMs = GetCurrentTimeMs();
    lastFeedMs_ = state_.lastInteractionMs;
    
    // 💝 记录情绪：喂食行为
    auto& emotional_memory = xiaozhi::EmotionalMemory::GetInstance();
    emotional_memory.RecordFeed();
    
    ESP_LOGI(TAG, "🍔 Fed pet +%d, Satiety: %d -> %d", amount * 2, 
             state_.satiety - amount * 2, state_.satiety);
    
    SaveState();
    return true;
}

bool PetSystem::Clean() {
    if (!CheckCooldown(lastCleanMs_, CLEAN_COOLDOWN_SEC)) {
        ESP_LOGW(TAG, "Clean on cooldown");
        return false;
    }
    
    if (state_.cleanliness >= 90) {
        ESP_LOGI(TAG, "🛁 Pet is already clean!");
        return false;
    }
    
    state_.cleanliness = Clamp(state_.cleanliness + 15);
    state_.mood = Clamp(state_.mood + 5);
    
    // 完成每日任务
    if (!(state_.dailyDoneMask & (1 << TASK_CLEAN))) {
        state_.dailyDoneMask |= (1 << TASK_CLEAN);
        state_.active = Clamp(state_.active + 5);
        ESP_LOGI(TAG, "✅ Daily task: Clean completed! +5 active");
    }
    
    state_.lastInteractionMs = GetCurrentTimeMs();
    lastCleanMs_ = state_.lastInteractionMs;
    
    ESP_LOGI(TAG, "🛁 Cleaned pet, Cleanliness: %d, Mood: %d", 
             state_.cleanliness, state_.mood);
    
    SaveState();
    return true;
}

bool PetSystem::Play(const std::string& kind) {
    if (!CheckCooldown(lastPlayMs_, PLAY_COOLDOWN_SEC)) {
        ESP_LOGW(TAG, "Play on cooldown");
        return false;
    }
    
    state_.mood = Clamp(state_.mood + 8);
    state_.active = Clamp(state_.active + 3);
    state_.satiety = Clamp(state_.satiety - 3);  // 玩耍消耗体力
    
    // 完成每日任务
    if (!(state_.dailyDoneMask & (1 << TASK_PLAY))) {
        state_.dailyDoneMask |= (1 << TASK_PLAY);
        state_.active = Clamp(state_.active + 5);
        ESP_LOGI(TAG, "✅ Daily task: Play completed! +5 active");
    }
    
    state_.lastInteractionMs = GetCurrentTimeMs();
    lastPlayMs_ = state_.lastInteractionMs;
    
    // 💝 记录情绪：玩耍行为
    auto& emotional_memory = xiaozhi::EmotionalMemory::GetInstance();
    emotional_memory.RecordPlay();
    
    ESP_LOGI(TAG, "🎾 Played with pet (%s), Mood: %d, Active: %d", 
             kind.empty() ? "default" : kind.c_str(), state_.mood, state_.active);
    
    SaveState();
    return true;
}

bool PetSystem::Hug() {
    if (!CheckCooldown(lastHugMs_, HUG_COOLDOWN_SEC)) {
        return false;
    }
    
    state_.mood = Clamp(state_.mood + 5);
    state_.lastInteractionMs = GetCurrentTimeMs();
    lastHugMs_ = state_.lastInteractionMs;
    
    // 💝 记录情绪：拥抱行为（增加信任度）
    auto& emotional_memory = xiaozhi::EmotionalMemory::GetInstance();
    emotional_memory.RecordHug();
    
    ESP_LOGI(TAG, "🤗 Hugged pet, Mood: %d", state_.mood);
    return true;
}

void PetSystem::RecordChat() {
    chatCountToday_++;
    state_.lastInteractionMs = GetCurrentTimeMs();
    
    // 聊天5次完成每日任务
    if (chatCountToday_ >= 5 && !(state_.dailyDoneMask & (1 << TASK_CHAT))) {
        state_.dailyDoneMask |= (1 << TASK_CHAT);
        state_.mood = Clamp(state_.mood + 5);
        state_.active = Clamp(state_.active + 5);
        ESP_LOGI(TAG, "✅ Daily task: Chat completed! +5 mood, +5 active");
        SaveState();
    }
}

int PetSystem::GetOverallState() const {
    return static_cast<int>(0.5f * state_.mood + 0.3f * state_.satiety + 0.2f * state_.cleanliness);
}

std::string PetSystem::GetRecommendedEmotion() const {
    // 优先判断特殊状态
    if (state_.satiety < 20) {
        return "sad";  // 饿了
    }
    
    if (state_.cleanliness < 20) {
        return "embarrassed";  // 脏了
    }
    
    // 根据综合状态判断
    int overall = GetOverallState();
    
    if (overall > 70) {
        return "happy";
    } else if (overall < 30) {
        return "sad";
    } else if (state_.mood < 40) {
        return "thinking";  // 情绪低落，需要关注
    } else {
        return "neutral";
    }
}

std::string PetSystem::GetStatusDescription() const {
    cJSON* root = cJSON_CreateObject();
    
    // 宠物类型信息
    auto pet_type = GetCurrentPetType();
    if (pet_type) {
        cJSON* petInfo = cJSON_CreateObject();
        cJSON_AddStringToObject(petInfo, "id", state_.petType.c_str());
        cJSON_AddStringToObject(petInfo, "name", pet_type->name.c_str());
        cJSON_AddStringToObject(petInfo, "emoji", pet_type->emoji.c_str());
        cJSON_AddStringToObject(petInfo, "category", pet_type->category.c_str());
        cJSON_AddStringToObject(petInfo, "personality", pet_type->personality.c_str());
        cJSON_AddStringToObject(petInfo, "special_trait", pet_type->special_trait.c_str());
        cJSON_AddItemToObject(root, "pet_type", petInfo);
    }
    
    cJSON_AddNumberToObject(root, "mood", state_.mood);
    cJSON_AddNumberToObject(root, "satiety", state_.satiety);
    cJSON_AddNumberToObject(root, "cleanliness", state_.cleanliness);
    cJSON_AddNumberToObject(root, "active", state_.active);
    cJSON_AddNumberToObject(root, "level", state_.level);
    cJSON_AddNumberToObject(root, "overall", GetOverallState());
    cJSON_AddStringToObject(root, "pet_emotion", GetRecommendedEmotion().c_str());
    
    // 每日任务状态
    cJSON* dailyTasks = cJSON_CreateObject();
    cJSON_AddBoolToObject(dailyTasks, "feed", state_.dailyDoneMask & (1 << TASK_FEED));
    cJSON_AddBoolToObject(dailyTasks, "clean", state_.dailyDoneMask & (1 << TASK_CLEAN));
    cJSON_AddBoolToObject(dailyTasks, "play", state_.dailyDoneMask & (1 << TASK_PLAY));
    cJSON_AddBoolToObject(dailyTasks, "chat", state_.dailyDoneMask & (1 << TASK_CHAT));
    cJSON_AddItemToObject(root, "daily_tasks", dailyTasks);
    
    cJSON_AddNumberToObject(root, "login_streak", state_.loginStreak);
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return result;
}

std::string PetSystem::GetSuggestions() const {
    std::string suggestions;
    
    if (state_.satiety < 30) {
        suggestions += "宠物饿了，该喂食了！ ";
    }
    if (state_.cleanliness < 30) {
        suggestions += "宠物脏了，需要洗澡！ ";
    }
    if (state_.mood < 40) {
        suggestions += "宠物心情不好，陪它玩一会儿吧！ ";
    }
    
    int64_t sinceInteraction = (GetCurrentTimeMs() - state_.lastInteractionMs) / 1000 / 60;
    if (sinceInteraction > 60) {
        suggestions += "好久没互动了，宠物想你了！ ";
    }
    
    // 每日任务提示
    if (!(state_.dailyDoneMask & (1 << TASK_FEED))) {
        suggestions += "[任务: 喂食] ";
    }
    if (!(state_.dailyDoneMask & (1 << TASK_CLEAN))) {
        suggestions += "[任务: 洗澡] ";
    }
    if (!(state_.dailyDoneMask & (1 << TASK_PLAY))) {
        suggestions += "[任务: 玩耍] ";
    }
    if (!(state_.dailyDoneMask & (1 << TASK_CHAT))) {
        suggestions += "[任务: 聊天5次] ";
    }
    
    if (suggestions.empty()) {
        suggestions = "宠物状态良好，继续保持！";
    }
    
    return suggestions;
}

void PetSystem::ResetDaily() {
    state_.dailyDoneMask = 0;
    chatCountToday_ = 0;
    state_.lastResetDayMs = GetCurrentTimeMs();
    SaveState();
    ESP_LOGI(TAG, "🔄 Daily tasks reset manually");
}

void PetSystem::DebugSet(int mood, int satiety, int cleanliness) {
    state_.mood = Clamp(mood);
    state_.satiety = Clamp(satiety);
    state_.cleanliness = Clamp(cleanliness);
    SaveState();
    ESP_LOGI(TAG, "🔧 Debug set: Mood=%d, Satiety=%d, Clean=%d", 
             state_.mood, state_.satiety, state_.cleanliness);
}

void PetSystem::LoadState() {
    Settings settings("pet", true);
    
    state_.petType = settings.GetString("pet_type", "cat");
    state_.mood = settings.GetInt("mood", 70);
    state_.satiety = settings.GetInt("satiety", 70);
    state_.cleanliness = settings.GetInt("cleanliness", 70);
    state_.active = settings.GetInt("active", 50);
    state_.level = settings.GetInt("level", 1);
    state_.lastUpdateMs = settings.GetInt64("last_ts", GetCurrentTimeMs());
    state_.lastInteractionMs = settings.GetInt64("last_interact", GetCurrentTimeMs());
    state_.dailyDoneMask = settings.GetInt("daily_done", 0);
    state_.loginStreak = settings.GetInt("login_streak", 0);
    state_.lastResetDayMs = settings.GetInt64("last_reset_day", GetCurrentTimeMs());
    
    // 验证宠物类型是否存在，不存在则使用默认猫咪
    if (pet_types_.find(state_.petType) == pet_types_.end()) {
        ESP_LOGW(TAG, "Invalid pet type '%s', reset to 'cat'", state_.petType.c_str());
        state_.petType = "cat";
    }
    
    ClampState();
    
    auto pet_type = GetCurrentPetType();
    ESP_LOGI(TAG, "📥 Loaded pet state from NVS: %s %s", 
             pet_type ? pet_type->emoji.c_str() : "",
             pet_type ? pet_type->name.c_str() : "");
}

void PetSystem::SaveState() {
    Settings settings("pet", true);
    
    settings.SetString("pet_type", state_.petType);
    settings.SetInt("mood", state_.mood);
    settings.SetInt("satiety", state_.satiety);
    settings.SetInt("cleanliness", state_.cleanliness);
    settings.SetInt("active", state_.active);
    settings.SetInt("level", state_.level);
    settings.SetInt64("last_ts", GetCurrentTimeMs());
    settings.SetInt64("last_interact", state_.lastInteractionMs);
    settings.SetInt("daily_done", state_.dailyDoneMask);
    settings.SetInt("login_streak", state_.loginStreak);
    settings.SetInt64("last_reset_day", state_.lastResetDayMs);
    
    // 🧠 发布宠物状态变化事件（供学习系统使用）
    xiaozhi::PetStateEventData event_data = {
        .mood = state_.mood,
        .satiety = state_.satiety,
        .cleanliness = state_.cleanliness,
        .overall = GetOverallState()
    };
    xiaozhi::EventBus::GetInstance().PublishNonBlocking(
        xiaozhi::PET_EVENT,
        xiaozhi::PET_STATE_CHANGED,
        &event_data,
        sizeof(event_data)
    );
    
    // ESP_LOGI(TAG, "💾 Saved pet state to NVS");
}

int PetSystem::Clamp(int value, int min, int max) const {
    return std::max(min, std::min(max, value));
}

bool PetSystem::CheckCooldown(int64_t& lastActionMs, int cooldownSeconds) const {
    int64_t now = GetCurrentTimeMs();
    int64_t elapsed = (now - lastActionMs) / 1000;
    
    if (elapsed < cooldownSeconds) {
        return false;
    }
    
    return true;
}

int64_t PetSystem::GetCurrentTimeMs() const {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t time_ms = (int64_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
    
    // 🛡️ 基本合理性检查（时间不应该是负数或明显错误）
    if (time_ms < 0) {
        ESP_LOGE(TAG, "❌ Invalid system time: %lld", time_ms);
        return 0;
    }
    
    return time_ms;
}

int PetSystem::GetDaysSince1970(int64_t timestampMs) const {
    return (int)(timestampMs / 1000 / 86400);
}

bool PetSystem::SelectPetType(const std::string& type_name) {
    // 🛡️ 输入验证：检查字符串格式
    if (type_name.empty() || type_name.length() > 32) {
        ESP_LOGE(TAG, "❌ Invalid pet type name length: %zu", type_name.length());
        return false;
    }
    
    // 🛡️ 验证字符串内容（只允许字母、数字和下划线）
    for (char c : type_name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            ESP_LOGE(TAG, "❌ Invalid character in pet type name: '%c'", c);
            return false;
        }
    }
    
    auto it = pet_types_.find(type_name);
    if (it == pet_types_.end()) {
        ESP_LOGW(TAG, "Pet type not found: %s", type_name.c_str());
        return false;
    }
    
    if (!it->second.is_available) {
        ESP_LOGW(TAG, "Pet type not available: %s", type_name.c_str());
        return false;
    }
    
    std::string old_type = state_.petType;
    state_.petType = type_name;
    SaveState();
    
    ESP_LOGI(TAG, "🔄 Changed pet type: %s -> %s %s", 
             old_type.c_str(), 
             it->second.emoji.c_str(),
             it->second.name.c_str());
    
    // 更新显示的宠物图标
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    if (display) {
        display->SetPetEmoji(it->second.emoji.c_str());
        ESP_LOGI(TAG, "Updated pet icon to %s", it->second.emoji.c_str());
    }
    
    return true;
}

const PetSystem::PetType* PetSystem::GetCurrentPetType() const {
    auto it = pet_types_.find(state_.petType);
    if (it != pet_types_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string PetSystem::ListPetTypes() const {
    cJSON* root = cJSON_CreateObject();
    
    // 按分类组织
    cJSON* domestic = cJSON_CreateArray();
    cJSON* wild = cJSON_CreateArray();
    cJSON* exotic = cJSON_CreateArray();
    
    for (const auto& pair : pet_types_) {
        if (!pair.second.is_available) continue;
        
        cJSON* type = cJSON_CreateObject();
        cJSON_AddStringToObject(type, "id", pair.first.c_str());
        cJSON_AddStringToObject(type, "name", pair.second.name.c_str());
        cJSON_AddStringToObject(type, "name_en", pair.second.name_en.c_str());
        cJSON_AddStringToObject(type, "emoji", pair.second.emoji.c_str());
        cJSON_AddStringToObject(type, "category", pair.second.category.c_str());
        cJSON_AddNumberToObject(type, "hunger_rate", pair.second.hunger_rate);
        cJSON_AddNumberToObject(type, "clean_rate", pair.second.clean_rate);
        cJSON_AddNumberToObject(type, "mood_decay_rate", pair.second.mood_decay_rate);
        cJSON_AddStringToObject(type, "personality", pair.second.personality.c_str());
        cJSON_AddStringToObject(type, "special_trait", pair.second.special_trait.c_str());
        
        if (pair.second.category == "domestic") {
            cJSON_AddItemToArray(domestic, type);
        } else if (pair.second.category == "wild") {
            cJSON_AddItemToArray(wild, type);
        } else {
            cJSON_AddItemToArray(exotic, type);
        }
    }
    
    cJSON_AddItemToObject(root, "domestic", domestic);
    cJSON_AddItemToObject(root, "wild", wild);
    cJSON_AddItemToObject(root, "exotic", exotic);
    cJSON_AddStringToObject(root, "current", state_.petType.c_str());
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return result;
}

std::string PetSystem::GetPetTypeInfo(const std::string& type_name) const {
    auto it = pet_types_.find(type_name);
    if (it == pet_types_.end()) {
        return "{\"error\":\"Pet type not found\"}";
    }
    
    const auto& pet = it->second;
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", type_name.c_str());
    cJSON_AddStringToObject(root, "name", pet.name.c_str());
    cJSON_AddStringToObject(root, "name_en", pet.name_en.c_str());
    cJSON_AddStringToObject(root, "emoji", pet.emoji.c_str());
    cJSON_AddStringToObject(root, "category", pet.category.c_str());
    cJSON_AddNumberToObject(root, "hunger_rate", pet.hunger_rate);
    cJSON_AddNumberToObject(root, "clean_rate", pet.clean_rate);
    cJSON_AddNumberToObject(root, "mood_decay_rate", pet.mood_decay_rate);
    cJSON_AddStringToObject(root, "personality", pet.personality.c_str());
    cJSON_AddStringToObject(root, "special_trait", pet.special_trait.c_str());
    cJSON_AddBoolToObject(root, "is_available", pet.is_available);
    cJSON_AddBoolToObject(root, "is_current", type_name == state_.petType);
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return result;
}

void PetSystem::SetWarningCallback(std::function<void(const std::string&)> callback) {
    warning_callback_ = std::move(callback);
    ESP_LOGI(TAG, "Pet warning callback registered");
}

void PetSystem::TriggerWarning(const std::string& warning) {
    if (warning_callback_) {
        ESP_LOGI(TAG, "🎯 Manually triggering pet warning: %s", warning.c_str());
        warning_callback_(warning);
    } else {
        ESP_LOGW(TAG, "⚠️  Warning callback not set, cannot trigger warning");
    }
}

void PetSystem::EnableAutoAnnouncement(bool enable, int interval_min) {
    auto_announcement_enabled_ = enable;
    auto_announcement_interval_min_ = interval_min;
    
    ESP_LOGI(TAG, "🔔 Auto announcement: %s (interval: %d min)", 
             enable ? "ENABLED" : "DISABLED", interval_min);
    
    if (enable) {
        // 重置计时器，避免立即触发
        last_announcement_ms_ = GetCurrentTimeMs();
    }
}

std::string PetSystem::CheckWarning() {
    auto pet_type = GetCurrentPetType();
    if (!pet_type) {
        return "";
    }
    
    // 🧠 时段感知：判断是否应该抑制警告
    auto& adaptive = xiaozhi::AdaptiveBehavior::GetInstance();
    if (adaptive.ShouldSuppressPetWarning()) {
        ESP_LOGD(TAG, "🔕 Pet warning suppressed by context (night/work time)");
        return "";
    }
    
    std::string warning_type;
    std::string message;
    
    // 🎯 获取自适应警告阈值偏移
    int threshold_offset = adaptive.GetPetWarningThresholdOffset();
    int adjusted_satiety_threshold = WARNING_SATIETY_THRESHOLD + threshold_offset;
    int adjusted_clean_threshold = WARNING_CLEAN_THRESHOLD + threshold_offset;
    int adjusted_mood_threshold = WARNING_MOOD_THRESHOLD + threshold_offset;
    
    // 优先级：饱腹度 > 清洁度 > 心情
    if (state_.satiety <= adjusted_satiety_threshold) {
        warning_type = "hunger";
        // 🎨 概率性反应（基于心情生成不同话语）
        message = adaptive.GetProbabilisticPetResponse(state_.mood, "hunger");
        // 🛡️ 安全的字符串拼接
        message = BuildWarningMessage(pet_type, message);
    } else if (state_.cleanliness <= adjusted_clean_threshold) {
        warning_type = "clean";
        // 🎨 概率性反应
        message = adaptive.GetProbabilisticPetResponse(state_.mood, "clean");
        message = BuildWarningMessage(pet_type, message);
    } else if (state_.mood <= adjusted_mood_threshold) {
        warning_type = "mood";
        // 🎨 概率性反应
        message = adaptive.GetProbabilisticPetResponse(state_.mood, "mood");
        message = BuildWarningMessage(pet_type, message);
    }
    
    // 如果有警告且不同于上次警告类型，返回消息
    if (!warning_type.empty() && warning_type != lastWarningType_) {
        ESP_LOGD(TAG, "🐾 Pet warning: type=%s, mood=%d, threshold_offset=%+d, message=%s",
                 warning_type.c_str(), state_.mood, threshold_offset, message.c_str());
        return message;
    }
    
    return "";
}

std::string PetSystem::BuildWarningMessage(const PetType* pet_type, const std::string& base_message) {
    if (!pet_type) {
        return base_message;  // 如果宠物类型为空，直接返回原消息
    }
    
    // 🛡️ 安全的字符串拼接，避免多次内存分配和异常
    std::string result;
    result.reserve(base_message.length() + pet_type->emoji.length() + pet_type->name.length() + 10);
    result = "主人，";
    result += pet_type->emoji;
    result += pet_type->name;
    
    // 🛡️ 安全的 substr：检查 find 结果
    size_t comma_pos = base_message.find("，");
    if (comma_pos != std::string::npos && comma_pos + 3 < base_message.length()) {
        result += base_message.substr(comma_pos + 3);
    } else {
        // fallback：如果找不到逗号，保留原消息
        result += base_message;
    }
    
    return result;
}

void PetSystem::CheckAndNotifyWarnings() {
    // 自动播报功能已禁用，仅支持手动查询（用户说"宠物状态"）
    // CheckWarning() 方法仍然可用于 MCP 工具调用
}

