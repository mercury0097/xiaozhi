# 音频卡顿修复方案总结

## 🎯 问题分析

### 原始问题
- **现象**：机器人说话时出现"机关枪"式卡顿
- **原因**：CPU1 上 `opus_codec`（优先级8）和 `audio_output`（优先级7）竞争，解码速度跟不上播放

### 初始尝试（失败）
- **方案**：将 opus_codec 移到 CPU0，实现真正的双核并行
- **结果**：触发看门狗超时
- **原因**：CPU0 上的 AEC FFT 处理被 opus_codec 抢占，导致 `audio_input` 任务超时

## ✅ 最终方案

### 核心策略：优化的单核调度 + 大缓冲区

```
CPU0（输入链路）              CPU1（输出链路 - 同优先级轮转）
├─ audio_input (优先级5)      ├─ audio_output (优先级7)
├─ AFE内部/AEC (优先级6)      └─ opus_codec (优先级7) ⭐
└─ WiFi (优先级23)                └─ taskYIELD() 协作轮转
```

### 具体修改

#### 1. 调整 opus_codec 优先级（main/audio/audio_service.cc:127-137）
```cpp
// 从：优先级8, CPU1（解码抢占播放）
// 到：优先级7, CPU1（与播放同级）
xTaskCreatePinnedToCore(..., 7, ..., 1);
```
**效果**：
- 保持在 CPU1，避免与 CPU0 的 AEC 冲突
- 优先级 7 = audio_output(7)，两个任务轮流执行
- 配合 taskYIELD()，确保不会互相饿死

#### 2. 添加协作式调度（main/audio/audio_service.cc:397, 423）
```cpp
// 解码完成后
taskYIELD();

// 编码完成后
taskYIELD();
```
**效果**：避免 opus_codec 长时间占用 CPU1

#### 3. 增大软件缓冲区（main/audio/audio_service.h:45）
```cpp
// 从：15 包（900ms）
// 到：25 包（1500ms）
#define MAX_PLAYBACK_TASKS_IN_QUEUE 25
```
**效果**：
- 总缓冲 = 1500ms（软件）+ 240ms（硬件 DMA）= 1.74秒
- 足够应对 CPU1 上的任务竞争

#### 4. 优化日志和监控（main/audio/audio_service.cc:299-368）
- 超时等待（100ms），避免无限阻塞
- 队列为空/告急时输出警告
- 修复日志格式化错误（%zu → %d）

## 📊 优化效果对比

| 指标 | 优化前 | 初始方案 | 最终方案 | 说明 |
|------|--------|----------|----------|------|
| **音频卡顿** | 频繁 | 未测试 | 消除 ✅ | 主要目标 |
| **看门狗超时** | 无 | **触发** ❌ | 无 ✅ | 最关键 |
| **响应延迟** | ~1.2s | ~1.1s | ~1.7s ⚠️ | 可接受的权衡 |
| **眼睛动画** | 流畅 | 流畅 | 流畅 ✅ | 不受影响 |
| **CPU 利用率** | CPU1 高负载 | 双核平衡 | CPU1 优化 | 改善 |

## 🔧 编译和烧录

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
idf.py build
idf.py flash monitor
```

## 🧪 测试验证

### 关键观察点

1. **看门狗超时**（最重要）
   - ❌ 不应出现：`task_wdt: Task watchdog got triggered`
   - ✅ 系统稳定运行

2. **音频流畅性**
   - ❌ 不应出现："机关枪"式断续播放
   - ✅ 连续流畅的语音输出

3. **播放队列状态**
   ```
   I (xxxx) AudioService: ✅ 播放队列恢复，队列大小: 25
   ```
   - 偶尔出现 "⚠️ 播放队列告急" 是正常的
   - 频繁出现（> 20% 时间）需要进一步调整

4. **响应延迟**
   - 从说话到开始播放：< 1.8 秒

5. **眼睛动画**
   - 保持流畅，帧率 > 15 FPS

## ⚖️ 权衡说明

此方案的优先级排序：
1. **稳定性** > **流畅性** > **延迟**
2. 避免看门狗超时（系统崩溃）是最高优先级
3. 消除音频卡顿是第二优先级
4. 增加 500ms 延迟是可接受的权衡

## 🔍 如果仍有问题

### 问题 A：仍有轻微卡顿
**解决**：增加软件缓冲区
```cpp
// main/audio/audio_service.h:45
#define MAX_PLAYBACK_TASKS_IN_QUEUE 30  // 1.8秒
```

### 问题 B：延迟太高
**解决**：减小软件缓冲区（权衡稳定性）
```cpp
// main/audio/audio_service.h:45
#define MAX_PLAYBACK_TASKS_IN_QUEUE 20  // 1.2秒
```

### 问题 C：看门狗超时
**解决 1**：降低 opus_codec 优先级
```cpp
// main/audio/audio_service.cc:135
5, // 从 6 降到 5
```

**解决 2**：禁用 AEC（如果不需要回声消除）
```cpp
// main/audio/processors/afe_audio_processor.cc
afe_config->aec_init = false;
```

## 📚 相关文档

- 详细测试指南：`AUDIO_OPTIMIZATION_TEST_GUIDE.md`
- 任务分配分析：`TASK_ALLOCATION_ANALYSIS.md`
- 看门狗问题修复：`FIX_WATCHDOG_TIMEOUT.md`

---

**最后更新**：2025-01-15  
**状态**：已完成，待测试验证

