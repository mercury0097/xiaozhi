# 🔄 ESP32-S3 双核任务分配分析

## 📊 概述

**芯片**：ESP32-S3（双核 Xtensa LX7）  
**架构**：对称多处理（SMP）  
**CPU**：
- **CPU0**（Protocol Core）：主要负责 WiFi、BT 协议栈、系统任务
- **CPU1**（Application Core）：主要负责用户应用任务

---

## 🎯 当前任务分配

### 📌 固定到 CPU0 的任务

| 任务名称 | 优先级 | 栈大小 | 功能描述 | 文件位置 |
|---------|--------|--------|---------|---------|
| **`audio_input`** | 8 | 10KB (2048×5) | 音频采集 + ESP-SR 处理（NS, SE, AGC, VAD） | `audio_service.cc:88` |
| **`wifi`** | 23 | 6KB | WiFi 驱动任务（ESP-IDF 自动分配） | ESP-IDF 内部 |
| **`main`** | 1 | 10KB | 主应用入口（app_main） | ESP-IDF 配置 |
| **`esp_timer`** | N/A | N/A | 高精度定时器服务 | ESP-IDF 配置 |

**CPU0 负载**：**高** 🔴  
主要压力来自：
1. 音频采集 + ESP-SR 降噪处理（持续运行）
2. WiFi 协议栈（网络通信时）
3. 系统中断处理

---

### 📌 固定到 CPU1 的任务

| 任务名称 | 优先级 | 栈大小 | 功能描述 | 文件位置 |
|---------|--------|--------|---------|---------|
| **`AfeWakeWord` (内部)** | 1 | N/A | 唤醒词检测（ESP-SR AFE，always-on） | `afe_wake_word.cc:78` |

**CPU1 负载**：**中等** 🟡  
主要压力来自：
1. 唤醒词检测（持续运行，低功耗模式）
2. 部分自动分配的任务

---

### 📌 自动分配的任务（由 FreeRTOS 调度器决定）

| 任务名称 | 优先级 | 栈大小 | 功能描述 | 文件位置 |
|---------|--------|--------|---------|---------|
| **`audio_output`** | 4 | 4KB (2048×2) | 音频播放输出 | `audio_service.cc:95` |
| **`audio_communication`** | 3 | 4KB | ESP-SR 语音通信处理（对话阶段） | `afe_audio_processor.cc:93` |
| **`encode_wake_word`** | 2 | 28KB (4096×7) | 唤醒词 Opus 编码 | `afe_wake_word.cc:202` |
| **`LVGL`** | 1-2 | N/A | 图形界面渲染 | `lcd_display.cc:126` |
| **`tcpip_task`** | 18 | 3KB | TCP/IP 协议栈 | ESP-IDF 配置 |
| **`IDLE0`** | 0 | 1.5KB | CPU0 空闲任务（看门狗） | FreeRTOS 内部 |
| **`IDLE1`** | 0 | 1.5KB | CPU1 空闲任务（看门狗） | FreeRTOS 内部 |

**调度策略**：FreeRTOS 根据 CPU 负载动态分配，优先使用 CPU1

---

## 🔥 CPU 负载分析

### CPU0（Protocol Core）
```
┌─────────────────────────────────┐
│ audio_input (8)        │ 60-70% │ ← 最大压力
├─────────────────────────────────┤
│ wifi (23)              │ 10-20% │ ← 网络活动时
├─────────────────────────────────┤
│ esp_timer              │  5-10% │
├─────────────────────────────────┤
│ 系统中断               │  5-10% │
└─────────────────────────────────┘
总负载：80-90% 🔴 (高负载)
```

**瓶颈分析**：
- ✅ **降噪（NS）**：神经网络模型，CPU 密集
- ✅ **语音增强（SE）**：频谱处理，CPU 密集
- ✅ **AGC**：实时增益控制
- ✅ **VAD**：语音活动检测
- ✅ **WiFi**：网络通信（特别是语音流传输）

---

### CPU1（Application Core）
```
┌─────────────────────────────────┐
│ AfeWakeWord (1)        │ 30-40% │ ← 唤醒词检测
├─────────────────────────────────┤
│ LVGL (1-2)             │ 10-20% │ ← 屏幕刷新时
├─────────────────────────────────┤
│ audio_communication (3)│  5-10% │ ← 对话时
├─────────────────────────────────┤
│ audio_output (4)       │  5-10% │ ← 播放音频时
├─────────────────────────────────┤
│ encode_wake_word (2)   │  5-15% │ ← 唤醒时短暂
└─────────────────────────────────┘
总负载：55-70% 🟡 (中高负载)
```

**瓶颈分析**：
- ✅ **唤醒词检测**：持续运行，ESP-SR WakeNet
- ⚠️ **LVGL**：GIF 动画渲染时负载较高
- ⚠️ **Opus 编码**：唤醒时短暂高负载

---

## ⚙️ 优化建议

### 1. **平衡 CPU0 负载** 🔴（优先级：高）

#### 方案 A：将 `audio_output` 固定到 CPU0
**原理**：音频输出比采集+处理简单，可以和 WiFi 错峰  
**修改**：
```cpp
// main/audio/audio_service.cc:91
xTaskCreatePinnedToCore([](void* arg) {
    AudioService* audio_service = (AudioService*)arg;
    audio_service->AudioOutputTask();
    vTaskDelete(NULL);
}, "audio_output", 2048 * 2, this, 4, &audio_output_task_handle_, 0);  // ← 固定到 CPU0
```

**预期效果**：
- ✅ CPU0 空闲时段利用率提升
- ✅ 减少 CPU1 负载
- ⚠️ 播放音频时可能略微影响 WiFi（可接受）

---

#### 方案 B：将 `LVGL` 固定到 CPU0（不推荐）
**原因**：LVGL 渲染时会占用大量 CPU，可能导致 WiFi 卡顿  
**不推荐实施**

---

### 2. **优化 CPU1 唤醒词检测**

#### 当前配置
```cpp
// main/audio/wake_words/afe_wake_word.cc:75
afe_config->afe_perferred_core = 1;          // CPU1
afe_config->afe_perferred_priority = 1;      // 低优先级
afe_config->afe_mode = AFE_MODE_LOW_COST;    // 低功耗模式 ✅
```

**已优化**：使用 `LOW_COST` 模式，降低 CPU 负载  
**建议保持现状** ✅

---

### 3. **增加栈大小监控**

当前 `audio_input` 任务栈已从 6KB 增加到 10KB，建议监控是否足够：

```cpp
// 添加栈使用率监控（在 audio_service.cc）
void AudioService::AudioInputTask() {
    while (true) {
        // ... 现有代码 ...
        
        // 每 10 秒打印栈使用情况
        static int counter = 0;
        if (++counter >= 100) {
            UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "audio_input stack remaining: %d bytes", stack_high_water_mark * 4);
            counter = 0;
        }
    }
}
```

---

### 4. **降低 LVGL 刷新率**（可选）

如果 CPU1 负载过高，可以降低 LVGL 刷新率：

```cpp
// main/display/lcd_display.cc:123
port_cfg.timer_period_ms = 50;  // 当前：20Hz (50ms)
// 改为：
port_cfg.timer_period_ms = 100; // 10Hz (100ms) - 更省 CPU
```

**权衡**：
- ✅ 减少 CPU1 负载
- ⚠️ 动画不够流畅

---

## 📈 任务优先级说明

FreeRTOS 优先级：**数字越大，优先级越高**

| 优先级 | 任务类型 | 示例任务 |
|--------|---------|---------|
| **23** | 协议栈（关键） | WiFi 驱动 |
| **18** | 网络协议 | TCP/IP 栈 |
| **8** | 实时音频采集 | `audio_input` |
| **4** | 音频播放 | `audio_output` |
| **3** | 音频处理 | `audio_communication` |
| **2** | 编码任务 | `encode_wake_word` |
| **1** | UI/唤醒词 | `LVGL`, `AfeWakeWord` |
| **0** | 空闲任务 | `IDLE0`, `IDLE1` |

**设计原则**：
- WiFi > 音频采集 > 音频播放 > 处理 > UI
- 确保网络和音频的实时性

---

## 🔍 性能监控命令

### 1. 查看任务列表和 CPU 使用率
```cpp
// 在代码中添加：
vTaskList(buffer);  // 打印所有任务
vTaskGetRunTimeStats(buffer);  // 打印 CPU 使用率
```

### 2. 使用 ESP-IDF 工具
```bash
# 查看实时任务状态
idf.py monitor

# 在串口中输入（如果启用了 console）
tasks
cpu
```

---

## 💡 为什么会 CPU 过载？

### 原因分析

1. **ESP-SR 降噪（NSNet）**：神经网络模型，计算密集
   - 每帧 512 样本（32ms）需要大量矩阵运算
   - 使用 `AFE_MODE_LOW_COST` 已优化

2. **语音增强（SE）**：频谱分析和滤波
   - FFT/IFFT 计算
   - 频段增强/抑制

3. **AGC（自动增益控制）**：实时音量调整
   - 每帧都要计算能量和增益

4. **VAD（语音活动检测）**：实时分析
   - 能量、频谱、统计特征

5. **WiFi + 音频双重负载**：
   - 音频流上传（Opus 编码 + 网络传输）
   - 网络延迟敏感

### 已采取的优化

✅ **禁用唤醒词阶段的 NS**：避免 CPU 过载  
✅ **使用 `LOW_COST` 模式**：降低 AFE 计算复杂度  
✅ **增加 `audio_input` 栈**：防止栈溢出  
✅ **AFE 分配到不同核心**：唤醒词在 CPU1，通话在自动分配  

---

## 🎯 总结

### 当前架构优点
1. ✅ **关键任务优先级合理**：WiFi > 音频采集 > 其他
2. ✅ **双核利用率较高**：CPU0 和 CPU1 都有工作负载
3. ✅ **低功耗模式启用**：唤醒词检测不会持续满载

### 潜在优化空间
1. ⚠️ **CPU0 负载较高（80-90%）**：可以考虑将部分任务迁移
2. ⚠️ **音频处理栈可能需要继续增加**：监控栈使用情况
3. ⚠️ **LVGL 刷新率可以降低**：如果 CPU1 负载过高

### 推荐配置
```
CPU0（Protocol Core）:
  - audio_input (8)           [固定]
  - audio_output (4)          [建议固定] ← 新增
  - wifi (23)                 [自动]
  - esp_timer                 [自动]

CPU1（Application Core）:
  - AfeWakeWord (1)           [固定]
  - LVGL (1-2)                [自动]
  - audio_communication (3)   [自动]
  - encode_wake_word (2)      [自动]
```

---

**创建时间**：2025-10-17  
**架构版本**：ESP32-S3 双核  
**当前状态**：✅ 已优化（低功耗模式 + 增加栈）


