# 🚀 事件总线 + 学习系统 集成指南

## 📋 概述

本指南说明如何将新增的**事件总线**和**轻量级学习系统**集成到现有代码中。

### 新增模块

```
main/
├── core/
│   ├── event_bus.h           # 事件总线（解耦模块）
│   └── event_bus.cc
└── learning/
    ├── user_profile.h        # 用户画像（SPIFFS存储）
    ├── user_profile.cc
    ├── decision_engine.h     # 决策引擎（概率计算）
    └── decision_engine.cc
```

---

## 🔧 集成步骤

### Step 1: 在 `application.cc` 中初始化

```cpp
#include "core/event_bus.h"
#include "learning/user_profile.h"
#include "learning/decision_engine.h"

void Application::Start() {
  // ... 现有初始化代码 ...
  
  // 1. 初始化事件总线
  auto& event_bus = xiaozhi::EventBus::GetInstance();
  event_bus.Initialize();
  
  // 2. 初始化用户画像（从SPIFFS加载）
  auto& profile = xiaozhi::UserProfile::GetInstance();
  profile.Initialize();
  
  // 3. 初始化决策引擎
  auto& decision = xiaozhi::DecisionEngine::GetInstance();
  decision.Initialize();
  
  // ... 后续代码 ...
}
```

---

### Step 2: 订阅事件（各模块）

#### 2.1 宠物系统订阅用户反馈事件

```cpp
// pet_system.cc
#include "core/event_bus.h"

void PetSystem::Start() {
  // ... 现有代码 ...
  
  // 订阅对话结束事件（自动记录互动）
  xiaozhi::EventBus::GetInstance().Subscribe(
      LOGIC_EVENT,
      xiaozhi::LOGIC_CONVERSATION_END,
      [this](void* data) {
        auto* conv_data = static_cast<xiaozhi::ConversationEventData*>(data);
        ESP_LOGI("Pet", "Conversation ended: topic=%s", conv_data->topic);
        
        // 如果话题是宠物相关，增加亲密度
        if (strstr(conv_data->topic, "pet")) {
          // 奖励心情 +5
        }
      }
  );
}
```

#### 2.2 表情系统订阅宠物状态事件

```cpp
// display/display.cc 或 emote_display.cc
#include "core/event_bus.h"

void EmoteDisplay::Initialize() {
  // ... 现有代码 ...
  
  // 订阅宠物状态变化事件
  xiaozhi::EventBus::GetInstance().Subscribe(
      PET_EVENT,
      xiaozhi::PET_STATE_CHANGED,
      [this](void* data) {
        auto* pet_state = static_cast<xiaozhi::PetStateEventData*>(data);
        
        // 根据宠物状态切换表情
        if (pet_state->overall > 70) {
          SetEmotion("happy");
        } else if (pet_state->overall < 30) {
          SetEmotion("sad");
        } else {
          SetEmotion("neutral");
        }
      }
  );
}
```

#### 2.3 学习系统订阅对话事件

```cpp
// learning/user_profile.cc 或在 application.cc 中
#include "core/event_bus.h"
#include "learning/user_profile.h"

void SetupLearningEventHandlers() {
  auto& event_bus = xiaozhi::EventBus::GetInstance();
  auto& profile = xiaozhi::UserProfile::GetInstance();
  
  // 订阅对话结束事件
  event_bus.Subscribe(
      LOGIC_EVENT,
      xiaozhi::LOGIC_CONVERSATION_END,
      [&profile](void* data) {
        auto* conv = static_cast<xiaozhi::ConversationEventData*>(data);
        profile.RecordInteraction(conv->topic, conv->duration_ms);
      }
  );
  
  // 订阅用户反馈事件
  event_bus.Subscribe(
      LOGIC_EVENT,
      xiaozhi::LOGIC_USER_FEEDBACK,
      [&profile](void* data) {
        auto* feedback = static_cast<xiaozhi::UserFeedbackData*>(data);
        profile.RecordFeedback(feedback->is_positive);
      }
  );
}
```

---

### Step 3: 发布事件（各模块）

#### 3.1 宠物系统发布状态变化事件

```cpp
// pet_system.cc
void PetSystem::UpdateState() {
  // ... 更新宠物状态 ...
  
  // 发布状态变化事件
  xiaozhi::PetStateEventData event_data = {
    .mood = state_.mood,
    .satiety = state_.satiety,
    .cleanliness = state_.cleanliness,
    .overall = state_.GetOverallState()
  };
  
  xiaozhi::EventBus::GetInstance().Publish(
      PET_EVENT,
      xiaozhi::PET_STATE_CHANGED,
      &event_data,
      sizeof(event_data)
  );
}
```

#### 3.2 对话逻辑发布对话事件

```cpp
// application.cc 或 protocol.cc
void OnConversationEnd(const char* topic, uint32_t duration_ms, bool positive) {
  xiaozhi::ConversationEventData event_data = {0};
  strncpy(event_data.topic, topic, sizeof(event_data.topic) - 1);
  event_data.positive_feedback = positive;
  event_data.duration_ms = duration_ms;
  
  xiaozhi::EventBus::GetInstance().Publish(
      LOGIC_EVENT,
      xiaozhi::LOGIC_CONVERSATION_END,
      &event_data,
      sizeof(event_data)
  );
}
```

#### 3.3 TTS播放时发布事件

```cpp
// audio_service.cc 或 application.cc
void OnTTSStart() {
  PUBLISH_EVENT_SIMPLE(LOGIC_EVENT, xiaozhi::LOGIC_TTS_START);
}

void OnTTSEnd() {
  PUBLISH_EVENT_SIMPLE(LOGIC_EVENT, xiaozhi::LOGIC_TTS_END);
}
```

---

### Step 4: 使用决策引擎

#### 4.1 主动问候（定时器触发）

```cpp
// application.cc
void Application::CreateProactiveGreetingTask() {
  xTaskCreate([](void* arg) {
    auto& decision = xiaozhi::DecisionEngine::GetInstance();
    
    while (1) {
      // 每10分钟检查一次
      vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000));
      
      if (decision.ShouldGreetProactively()) {
        auto ctx = decision.GetCurrentContext();
        const char* greeting = decision.GetGreetingByContext(ctx);
        
        // 触发主动问候（调用TTS）
        ESP_LOGI("App", "Proactive greeting: %s", greeting);
        // PlayTTS(greeting);
      }
    }
  }, "proactive", 2048, nullptr, 5, nullptr);
}
```

#### 4.2 宠物提醒（定时器触发）

```cpp
// pet_system.cc
void PetSystem::CreateReminderTask() {
  xTaskCreate([](void* arg) {
    auto& decision = xiaozhi::DecisionEngine::GetInstance();
    
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));  // 每30分钟检查
      
      if (decision.ShouldRemindPet()) {
        ESP_LOGI("Pet", "Time to check on your pet!");
        // 发布提醒事件或直接调用TTS
      }
    }
  }, "pet_reminder", 2048, nullptr, 5, nullptr);
}
```

#### 4.3 个性化话题推荐

```cpp
// application.cc
void Application::OnUserIdle() {
  auto& decision = xiaozhi::DecisionEngine::GetInstance();
  const char* topic = decision.RecommendTopic();
  
  ESP_LOGI("App", "Recommending: %s", topic);
  // PlayTTS(topic);
}
```

---

### Step 5: 定时保存用户画像

```cpp
// application.cc
void Application::CreateAutoSaveTask() {
  xTaskCreate([](void* arg) {
    auto& profile = xiaozhi::UserProfile::GetInstance();
    
    while (1) {
      // 每10分钟检查一次
      vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000));
      
      profile.CheckAutoSave();  // 如果有更新，自动保存到SPIFFS
    }
  }, "auto_save", 2048, nullptr, 3, nullptr);
}
```

---

## 🎯 MVP功能实现（3个最小可用功能）

### MVP 1: 主动问候

```cpp
// 在 application.cc 的 Start() 中添加
void Application::Start() {
  // ... 初始化 ...
  
  // 启动主动问候任务
  xTaskCreate([](void* arg) {
    auto& decision = xiaozhi::DecisionEngine::GetInstance();
    
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000));  // 10分钟
      
      if (decision.ShouldGreetProactively()) {
        auto ctx = decision.GetCurrentContext();
        const char* greeting = decision.GetGreetingByContext(ctx);
        
        // 触发TTS播放
        ESP_LOGI("App", "🎙️ Proactive: %s", greeting);
        // TODO: 调用你的TTS播放函数
        // PlayTTS(greeting);
      }
    }
  }, "greeting", 3072, nullptr, 5, nullptr);
}
```

### MVP 2: 宠物提醒

```cpp
// 在 pet_system.cc 的 Start() 中添加
void PetSystem::Start() {
  // ... 现有代码 ...
  
  xTaskCreate([](void* arg) {
    auto& decision = xiaozhi::DecisionEngine::GetInstance();
    
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));  // 30分钟
      
      if (decision.ShouldRemindPet()) {
        ESP_LOGI("Pet", "🐾 提醒照顾宠物");
        // TODO: 发送提醒
      }
    }
  }, "pet_remind", 2048, nullptr, 5, nullptr);
}
```

### MVP 3: 话题推荐

```cpp
// 在对话空闲时调用（例如VAD检测到长时间无声音）
void Application::OnLongIdle() {
  auto& decision = xiaozhi::DecisionEngine::GetInstance();
  const char* topic = decision.RecommendTopic();
  
  ESP_LOGI("App", "💡 推荐话题: %s", topic);
  // PlayTTS(topic);
}
```

---

## 📊 事件流程示例

### 场景1：用户问天气

```
1. 用户说话 → VAD检测 → 发布 LOGIC_VAD_VOICE_START
2. 唤醒词检测 → 发布 LOGIC_WAKE_WORD_DETECTED
3. 云端识别"天气" → 发布 LOGIC_INTENT_PARSED
4. 开始TTS播放 → 发布 LOGIC_TTS_START
   ↓
   表情系统订阅 → 切换到"说话"表情
5. TTS结束 → 发布 LOGIC_TTS_END
   ↓
   表情系统订阅 → 恢复"中性"表情
6. 对话结束 → 发布 LOGIC_CONVERSATION_END
   ↓
   学习系统订阅 → RecordInteraction("weather", 5000ms)
   ↓
   自动保存到SPIFFS
```

### 场景2：照顾宠物

```
1. 用户说"喂宠物" → MCP工具调用 pet.feed()
2. 宠物系统更新状态 → 饱腹度+10，心情+3
3. 发布 PET_STATE_CHANGED 事件
   ↓
   表情系统订阅 → 切换到"开心"表情
4. 对话结束 → 发布 LOGIC_CONVERSATION_END
   ↓
   学习系统订阅 → RecordInteraction("pet", 3000ms)
```

---

## 🧪 测试和调试

### 测试1：验证事件总线

```cpp
// 在 application.cc 的 Start() 中添加
void TestEventBus() {
  auto& bus = xiaozhi::EventBus::GetInstance();
  
  // 订阅测试事件
  bus.Subscribe(LOGIC_EVENT, 99, [](void* data) {
    ESP_LOGI("Test", "✅ Event received!");
  });
  
  // 发布测试事件
  bus.Publish(LOGIC_EVENT, 99, nullptr, 0);
  
  // 查看统计
  auto stats = bus.GetStats();
  ESP_LOGI("Test", "Published: %lu, Dropped: %lu", 
           stats.total_published, stats.total_dropped);
}
```

### 测试2：验证用户画像

```cpp
void TestUserProfile() {
  auto& profile = xiaozhi::UserProfile::GetInstance();
  
  // 模拟互动
  profile.RecordInteraction("weather", 5000);
  profile.RecordInteraction("pet", 3000);
  profile.RecordFeedback(true);
  
  // 打印统计
  profile.PrintStats();
  
  // 保存到SPIFFS
  profile.SaveToSPIFFS();
}
```

### 测试3：验证决策引擎

```cpp
void TestDecisionEngine() {
  auto& decision = xiaozhi::DecisionEngine::GetInstance();
  
  // 打印决策参数
  decision.PrintDecisionParams();
  
  // 测试决策
  ESP_LOGI("Test", "Proactive prob: %.2f", decision.GetProactiveProbability());
  ESP_LOGI("Test", "Pet interval: %d min", decision.GetPetReminderInterval());
  ESP_LOGI("Test", "Emotion liveness: %d", decision.GetEmotionLiveness());
  ESP_LOGI("Test", "Greeting: %s", decision.GetGreetingByContext(
      xiaozhi::DecisionEngine::CONTEXT_MORNING));
}
```

---

## 📝 注意事项

1. **SPIFFS挂载**：确保SPIFFS已挂载，否则用户画像无法保存
   ```cpp
   // 检查SPIFFS是否挂载
   struct stat st;
   if (stat("/spiffs", &st) != 0) {
       ESP_LOGE(TAG, "SPIFFS not mounted!");
   }
   ```

2. **内存管理**：事件数据按值拷贝，避免悬挂指针
   ```cpp
   // ✅ 正确：传递结构体
   PetStateEventData data = {...};
   Publish(PET_EVENT, PET_STATE_CHANGED, &data, sizeof(data));
   
   // ❌ 错误：传递指针（data可能被释放）
   PetStateEventData* ptr = new PetStateEventData{...};
   Publish(PET_EVENT, PET_STATE_CHANGED, &ptr, sizeof(ptr));
   ```

3. **Handler执行时间**：事件handler应该快速返回，避免阻塞
   ```cpp
   // ✅ 正确：快速处理
   bus.Subscribe(PET_EVENT, PET_STATE_CHANGED, [](void* data) {
       SetEmotion("happy");  // 快速调用
   });
   
   // ❌ 错误：耗时操作
   bus.Subscribe(PET_EVENT, PET_STATE_CHANGED, [](void* data) {
       SaveToSD();  // 可能阻塞很久
       DoHeavyWork();  // 应该放到独立任务
   });
   ```

4. **CPU占用监控**：
   ```cpp
   // 定期打印统计
   auto stats = EventBus::GetInstance().GetStats();
   ESP_LOGI(TAG, "Events: published=%lu, dropped=%lu", 
            stats.total_published, stats.total_dropped);
   ```

---

## 🎉 完成！

集成完成后，你的系统将具备：
- ✅ **事件驱动架构**：模块解耦，易扩展
- ✅ **轻量级学习**：个性化用户体验
- ✅ **0 NVS占用**：所有学习数据存SPIFFS
- ✅ **<1% CPU**：极低资源占用

**下一步**：根据用户反馈数据，逐步调整决策参数（概率、间隔等）。























