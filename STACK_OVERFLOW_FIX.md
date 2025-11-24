# 🔧 音频任务栈溢出修复

## ❌ 遇到的问题

### 错误现象

```
***ERROR*** A stack overflow in task audio_input has been detected.
W (18913) AFE: Ringbuffer of AFE(FEED) is full, Please use fetch() to read data
```

系统能正常启动，但对话一次后就崩溃重启。

---

## 🔍 根本原因

### 问题分析

1. ✅ **启动阶段稳定**：降低性能模式后，启动不再崩溃
2. ❌ **运行时栈溢出**：启用 WebRTC 降噪后，`audio_input` 任务栈空间不足
3. ⚠️ **数据积压**：音频处理速度跟不上输入速度

### 栈空间对比

| 配置 | 栈大小 | 状态 |
|------|--------|------|
| **无降噪** | 6144 字节 | ✅ 正常 |
| **WebRTC 降噪** | 6144 字节 | ❌ **溢出** |
| **WebRTC 降噪（修复后）** | 10240 字节 | ✅ 应该正常 |

---

## ✅ 解决方案

### 修改：增加音频任务栈大小

**文件**：`main/audio/audio_service.cc`

```cpp
// 修改前（会溢出）
}, "audio_input", 2048 * 3, this, 8, &audio_input_task_handle_, 0);
//                ^^^^^^^^ = 6144 字节

// 修改后（增加空间）
}, "audio_input", 2048 * 5, this, 8, &audio_input_task_handle_, 0);
//                ^^^^^^^^ = 10240 字节
```

**增加量**：`+4096` 字节（约 67% 增长）

---

## 🎯 预期效果

### 启动时

```
I (xxxx) AfeWakeWord: 唤醒词检测: 降噪已禁用（避免CPU过载）
I (xxxx) AFE: AFE Pipeline: ... -> |VAD(WebRTC)| -> |WakeNet(...)| -> ...
I (xxxx) Application: STATE: idle
```

### 对话时

```
I (xxxx) Application: Wake word detected: 你好小智
I (xxxx) AfeAudioProcessor: 使用 WebRTC 降噪（无需模型）
I (xxxx) AFE: AFE Pipeline: [input] -> |NS(WebRTC)| -> |VAD(...)| -> [output]
I (xxxx) Application: STATE: listening
```

**关键**：
- ✅ 不应该看到 "stack overflow"
- ✅ 可能仍有 "Ringbuffer full" 警告（但不应该崩溃）
- ✅ 对话应该正常完成

---

## 🚀 立即测试

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
idf.py build flash monitor
```

### 测试步骤

1. **启动**：观察是否稳定启动
2. **唤醒**：说"你好小智"
3. **对话**：完成一轮完整对话
4. **重复**：尝试多次对话，确认不再崩溃

---

## 📊 内存占用对比

| 任务 | 修改前 | 修改后 | 增加 |
|------|--------|--------|------|
| `audio_input` | 6 KB | 10 KB | +4 KB |
| `audio_output` | 4 KB | 4 KB | 0 |
| `opus_codec` | 26 KB | 26 KB | 0 |
| **总计** | 36 KB | 40 KB | +4 KB |

**影响**：增加了 4KB 栈空间（ESP32-S3 有 512KB SRAM，影响很小）

---

## ⚠️ 如果仍然有问题

### 场景 A：仍然栈溢出

**可能性**：极低（10KB 应该足够）

**解决**：
1. 进一步增加到 `2048 * 6` (12288 字节)
2. 或者禁用降噪（见下文）

### 场景 B：频繁出现 "Ringbuffer full" 警告

**可能性**：中等

**影响**：
- ⚠️ 可能会有轻微音频丢失
- ⚠️ 识别准确率可能降低
- ✅ 但系统不会崩溃

**优化**：
```cpp
// 在 afe_wake_word.cc 或 afe_audio_processor.cc 中
afe_config->afe_ringbuf_size = 100;  // 从默认 50 增加到 100
```

### 场景 C：还是想完全禁用降噪

**如果您觉得降噪改善不明显**，可以完全禁用：

编辑 `main/audio/processors/afe_audio_processor.cc`：

```cpp
// 找到这段代码（约第 48 行）
if (ns_model_name != nullptr) {
    // ... 神经网络降噪 ...
} else {
    // 没有模型时，使用 WebRTC 降噪（无需模型）
    afe_config->ns_init = true;
    afe_config->ns_model_name = nullptr;
    afe_config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
    ESP_LOGI(TAG, "使用 WebRTC 降噪（无需模型）");
}

// 改为：
if (ns_model_name != nullptr) {
    // 优先使用神经网络降噪模型
    afe_config->ns_init = true;
    afe_config->ns_model_name = ns_model_name;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    ESP_LOGI(TAG, "使用神经网络降噪模型: %s", ns_model_name);
} else {
    // 完全禁用降噪
    afe_config->ns_init = false;
    ESP_LOGI(TAG, "降噪已禁用（保持原始配置）");
}
```

---

## 💡 降噪效果评估

### 测试方法

启用降噪后，对比测试：

1. **安静环境**：
   - 说"你好小智" → 应该正常识别
   - 对话 → 识别准确率应该高

2. **噪音环境**（打开电风扇或播放音乐）：
   - 说"你好小智" → **可能更难触发**（唤醒词无降噪）
   - 对话 → **识别率应该提高**（语音识别有降噪）

3. **多次对话**：
   - 连续对话 3-5 次
   - 观察是否有崩溃或卡顿

### 判断标准

#### ✅ 降噪有效

- 噪音环境下识别率明显提高
- 没有崩溃或卡顿
- 偶尔有 "Ringbuffer full" 警告但不影响使用

#### ⚠️ 降噪效果不明显

- 识别率改善不大
- 频繁出现 "Ringbuffer full" 警告
- 系统不稳定

**建议**：考虑禁用降噪，保持原始配置。

#### ❌ 降噪导致问题

- 仍然崩溃
- 音频明显延迟或卡顿
- 识别率反而下降

**建议**：立即禁用降噪（见上文）。

---

## 🎯 最佳实践

### 推荐配置（稳定性优先）

```
✅ 唤醒词检测：AFE_MODE_LOW_COST + 无降噪
✅ 语音识别：AFE_MODE_LOW_COST + WebRTC 降噪
✅ audio_input 栈：10240 字节
```

### 高性能配置（效果优先，需要更强硬件）

```
⚠️ 唤醒词检测：AFE_MODE_HIGH_PERF + WebRTC 降噪
⚠️ 语音识别：AFE_MODE_HIGH_PERF + 神经网络降噪
⚠️ audio_input 栈：12288 字节
⚠️ 需要：更大功率电源 + 良好散热
```

### 保守配置（原始状态，最稳定）

```
✅ 唤醒词检测：AFE_MODE_HIGH_PERF + 无降噪
✅ 语音识别：AFE_MODE_HIGH_PERF + 无降噪
✅ audio_input 栈：6144 字节（原始值）
```

---

## 📝 总结

### 已完成的修复

| 问题 | 修复 | 状态 |
|------|------|------|
| 启动时 CPU 过载 | 降低性能模式 | ✅ 已解决 |
| 启动时降噪过载 | 禁用唤醒词降噪 | ✅ 已解决 |
| 音频任务栈溢出 | 增加栈大小到 10KB | ✅ 待验证 |

### 下一步

1. **立即**：编译测试，验证栈溢出是否解决
2. **观察**：多次对话，确认稳定性
3. **评估**：降噪效果是否明显
4. **决定**：是否保留降噪或恢复原始配置

---

**现在编译测试！** 🚀

```bash
idf.py build flash monitor
```

期待：
- ✅ 系统稳定运行
- ✅ 可以多次对话
- ✅ 降噪改善语音识别

如果仍有问题，请告诉我日志内容！



