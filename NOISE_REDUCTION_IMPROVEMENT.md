# 🎤 降噪改进说明

## ✅ 已完成的改进

### 问题诊断

通过检查工具发现：
- ✅ ESP-SR 库已编译链接
- ✅ 音频处理器配置已启用
- ✅ ESP-SR 在运行时正常工作
- ❌ **但是降噪功能未启用**（因为缺少降噪模型）

---

## 🔧 改进内容

### 修改 1：启用 WebRTC 降噪（语音识别阶段）

**文件**：`main/audio/processors/afe_audio_processor.cc`

**改动**：当没有神经网络降噪模型时，自动启用 WebRTC 降噪。

```cpp
// 之前：没有模型就完全禁用降噪
if (ns_model_name != nullptr) {
    afe_config->ns_init = true;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
} else {
    afe_config->ns_init = false;  // ← 导致没有任何降噪
}

// 现在：没有模型时使用 WebRTC 降噪
if (ns_model_name != nullptr) {
    // 优先使用神经网络降噪模型
    afe_config->ns_init = true;
    afe_config->ns_model_name = ns_model_name;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    ESP_LOGI(TAG, "使用神经网络降噪模型: %s", ns_model_name);
} else {
    // 没有模型时，使用 WebRTC 降噪（无需模型）
    afe_config->ns_init = true;
    afe_config->ns_model_name = nullptr;
    afe_config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
    ESP_LOGI(TAG, "使用 WebRTC 降噪（无需模型）");
}
```

---

### 修改 2：启用降噪（唤醒词检测阶段）

**文件**：`main/audio/wake_words/afe_wake_word.cc`

**改动**：为唤醒词检测阶段也添加降噪支持。

```cpp
// 新增降噪配置
char* ns_model_name = esp_srmodel_filter(models_, ESP_NSNET_PREFIX, NULL);
if (ns_model_name != nullptr) {
    // 优先使用神经网络降噪模型
    afe_config->ns_init = true;
    afe_config->ns_model_name = ns_model_name;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    ESP_LOGI(TAG, "唤醒词检测: 使用神经网络降噪模型 %s", ns_model_name);
} else {
    // 没有模型时，使用 WebRTC 降噪（无需模型）
    afe_config->ns_init = true;
    afe_config->ns_model_name = nullptr;
    afe_config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
    ESP_LOGI(TAG, "唤醒词检测: 使用 WebRTC 降噪（无需模型）");
}
```

---

## 🎯 预期效果

### 启动后您会看到：

```
I (6663) AfeWakeWord: 唤醒词检测: 使用 WebRTC 降噪（无需模型）
I (6733) AFE: AFE Pipeline: [input] -> |NS(WebRTC)| -> |VAD(WebRTC)| -> |WakeNet(...)| -> [output]
                                        ↑
                                  新增降噪模块！
```

说"你好小智"后：

```
I (170513) AfeAudioProcessor: 使用 WebRTC 降噪（无需模型）
I (170513) AFE: AFE Pipeline: [input] -> |NS(WebRTC)| -> |VAD(WebRTC)| -> [output]
                                          ↑
                                    语音识别也有降噪了！
```

---

## 📊 降噪效果对比

| 阶段 | 修改前 | 修改后 |
|------|--------|--------|
| **唤醒词检测** | ❌ 无降噪 | ✅ WebRTC 降噪 |
| **语音识别** | ❌ 无降噪 | ✅ WebRTC 降噪 |

### 效果预期

- ✅ **环境噪音抑制**：风扇、空调等稳态噪音
- ✅ **回声消除**：一定程度减少回声
- ✅ **语音清晰度**：提高识别准确率
- ⚠️ **资源占用**：略微增加（但 WebRTC 模式很轻量）

---

## 🚀 立即测试

### 1. 编译烧录

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
idf.py build flash monitor
```

### 2. 观察日志

**启动时**应该看到：

```
I (xxxx) AfeWakeWord: 唤醒词检测: 使用 WebRTC 降噪（无需模型）
```

**说"你好小智"后**应该看到：

```
I (xxxx) AfeAudioProcessor: 使用 WebRTC 降噪（无需模型）
```

### 3. 测试效果

1. **安静环境测试**：说"你好小智"，应该能正常识别
2. **噪音环境测试**：
   - 打开电风扇或音乐
   - 说"你好小智"，观察识别率是否提高
3. **日常对话测试**：对机器人说话，看是否能听得更清楚

---

## 💡 进一步优化（可选）

### 选项 1：调整 VAD 灵敏度

如果机器人过于敏感或迟钝，可以调整 VAD 模式：

```cpp
// afe_audio_processor.cc 第 42 行
afe_config->vad_mode = VAD_MODE_0;  // 当前值

// 可选值：
// VAD_MODE_0: 最激进（最容易检测到语音，但误报多）
// VAD_MODE_1: 较激进
// VAD_MODE_2: 平衡
// VAD_MODE_3: 保守（需要更清晰的语音，但误报少）
```

### 选项 2：添加神经网络降噪模型（效果更好）

如果您想要更好的降噪效果，可以下载 NS 模型：

#### 步骤 1：查看是否有模型

```bash
# 检查 assets 分区中是否有 NS 模型
idf.py monitor | grep -i "nsnet"
```

#### 步骤 2：获取模型（如果需要）

联系项目维护者或查看 ESP-SR 文档：
- 模型名称：`nsnet3_ch1` 或 `nsnet2`
- 放置位置：打包到 `srmodels.bin` 中

#### 步骤 3：验证

启动后应该看到：

```
I (xxxx) AfeAudioProcessor: 使用神经网络降噪模型: nsnet3_ch1
I (xxxx) AFE: AFE Pipeline: [input] -> |NS(Net)| -> |VAD(...)| -> [output]
                                        ↑
                                   神经网络降噪！
```

---

## 🔍 故障排除

### 问题 1：没有看到降噪日志

**可能原因**：编译缓存

**解决**：

```bash
idf.py fullclean
idf.py build flash monitor
```

### 问题 2：降噪效果不明显

**可能原因**：
1. WebRTC 降噪对某些噪音效果有限
2. 麦克风硬件问题
3. 噪音过大

**建议**：
1. 尝试调整 VAD 模式（见上文）
2. 检查麦克风连接
3. 考虑添加神经网络降噪模型

### 问题 3：系统变慢或崩溃

**可能原因**：降噪算法占用资源过多

**解决**：

```cpp
// afe_audio_processor.cc 第 40 行
// 将 AFE_MODE_HIGH_PERF 改为 AFE_MODE_LOW_COST
afe_config_t* afe_config = afe_config_init(..., AFE_MODE_LOW_COST);
```

或者禁用唤醒词阶段的降噪（保留语音识别阶段的降噪）。

---

## 📝 技术原理

### WebRTC 降噪

WebRTC 降噪是一种**实时音频处理算法**：

1. **频谱分析**：将音频分解为不同频率
2. **噪音估计**：在静音时估计噪音水平
3. **自适应滤波**：动态调整滤波器参数
4. **语音增强**：保留语音频率，抑制噪音频率

**优点**：
- ✅ 无需模型文件
- ✅ 资源占用低
- ✅ 实时性好

**缺点**：
- ⚠️ 对复杂噪音效果有限
- ⚠️ 不如神经网络降噪

### 神经网络降噪（Net）

使用深度学习模型进行降噪：

1. **训练数据**：大量干净语音 + 噪音样本
2. **模型推理**：实时预测哪些是噪音
3. **精准滤除**：更智能地区分语音和噪音

**优点**：
- ✅ 效果更好
- ✅ 适应性强

**缺点**：
- ⚠️ 需要模型文件（~500KB）
- ⚠️ 资源占用较高

---

## 🎉 总结

### 已解决的问题

- ✅ 找到了降噪未启用的根本原因
- ✅ 启用了 WebRTC 降噪（两个阶段）
- ✅ 添加了详细的日志输出
- ✅ 提供了进一步优化的建议

### 下一步

1. **立即测试**：编译烧录，验证效果
2. **反馈**：告诉我改进效果如何
3. **（可选）优化**：根据实际使用调整参数

---

**现在编译测试吧！** 🚀

```bash
idf.py build flash monitor
```

期待看到：
```
I (xxxx) AfeWakeWord: 唤醒词检测: 使用 WebRTC 降噪（无需模型）
I (xxxx) AfeAudioProcessor: 使用 WebRTC 降噪（无需模型）
```



