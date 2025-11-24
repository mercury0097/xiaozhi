# 🔍 ESP-SR 集成说明

## ⚠️ 为什么会冲突

您的项目已经有完整的音频系统，我们添加的代码和它冲突了：

### 冲突原因

1. **I2S 接口冲突**
   - 您的项目可能已经使用了 I2S_NUM_0
   - 我们的代码也用 I2S_NUM_0，导致冲突

2. **音频编解码器冲突**
   - 项目中有 `NoAudioCodec`、`BoxAudioCodec`、`ES8311AudioCodec` 等
   - 这些已经管理了音频硬件

3. **AFE 音频处理器冲突**
   - 项目中有 `AfeAudioProcessor` (已经使用 ESP-SR)
   - 我们又启动了一个，导致资源冲突

---

## ✅ 已恢复的文件

| 文件 | 状态 |
|------|------|
| `main/main.cc` | ✅ 已恢复原状 |
| `main/CMakeLists.txt` | ✅ 已恢复原状 |

---

## 🎯 您的项目已经有 ESP-SR！

查看您的代码结构：

```
main/audio/
├── audio_codec.cc          # 音频编解码器
├── audio_service.cc        # 音频服务
└── processors/
    ├── afe_audio_processor.cc   # ← ESP-SR AFE 已经在这里！
    └── afe_audio_processor.h
```

**您的项目已经集成了 ESP-SR AFE！**

---

## 📊 ESP-SR 在您项目中的使用

您的项目通过 `AfeAudioProcessor` 使用 ESP-SR：

```cpp
// 在 audio/processors/afe_audio_processor.cc 中
void AfeAudioProcessor::Initialize(AudioCodec* codec, ...) {
    // 已经配置了 ESP-SR AFE
    afe_config_t* afe_config = afe_config_init(...);
    afe_config->aec_mode = AEC_MODE_VOIP_HIGH_PERF;
    afe_config->vad_mode = VAD_MODE_0;
    
    if (ns_model_name != nullptr) {
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;  // 神经网络降噪
    }
    
    afe_iface_ = esp_afe_handle_from_config(afe_config);
    afe_data_ = afe_iface_->create_from_config(afe_config);
}
```

---

## 🔍 如何验证 ESP-SR 是否在工作

### 方法 1：查看编译日志

```bash
idf.py build 2>&1 | grep -i "afe\|esp-sr"
```

应该看到链接了 ESP-SR 库。

### 方法 2：查看运行日志

启动时应该有类似的日志：
```
I (xxx) AfeAudioProcessor: Audio communication task started
I (xxx) AfeAudioProcessor: feed size: 512, fetch size: 160
```

### 方法 3：查看配置

```bash
cat sdkconfig | grep AUDIO_PROCESSOR
```

应该看到：
```
CONFIG_USE_AUDIO_PROCESSOR=y
```

---

## 💡 为什么我的降噪不好？

可能的原因：

### 1️⃣ 神经网络降噪模型未加载

查看日志中是否有：
```
I (xxx) AfeAudioProcessor: Using ns model: nsnet3_ch1
```

如果没有这行，说明没有使用神经网络降噪。

### 2️⃣ VAD 模式设置过低

在 `afe_audio_processor.cc` 中：
```cpp
afe_config->vad_mode = VAD_MODE_0;  // 这个值太低了
```

**改进建议**：改为 `VAD_MODE_3` 以提高灵敏度。

### 3️⃣ 降噪未启用

检查配置中是否启用了降噪。

---

## 🛠️ 如何改进现有系统的降噪

### 选项 1：修改 AfeAudioProcessor 配置（推荐）

编辑 `main/audio/processors/afe_audio_processor.cc`：

```cpp
void AfeAudioProcessor::Initialize(...) {
    // ... 现有代码 ...
    
    // 修改 VAD 模式（提高灵敏度）
    afe_config->vad_mode = VAD_MODE_3;  // 从 VAD_MODE_0 改为 VAD_MODE_3
    
    // 确保启用降噪
    if (ns_model_name != nullptr) {
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
        ESP_LOGI(TAG, "✅ 使用神经网络降噪: %s", ns_model_name);
    } else {
        // 如果没有神经网络模型，使用传统降噪
        afe_config->ns_init = true;
        afe_config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
        ESP_LOGI(TAG, "✅ 使用 WEBRTC 降噪");
    }
    
    // ... 其余代码 ...
}
```

### 选项 2：调整麦克风硬件

- 检查麦克风连接是否牢固
- 确认麦克风型号和配置
- 调整麦克风位置（远离噪声源）

### 选项 3：查看音频编解码器配置

查看使用的是哪个编解码器：
```
main/audio/codecs/
├── no_audio_codec.cc
├── box_audio_codec.cc
├── es8311_audio_codec.cc
└── ...
```

确保编解码器配置正确（采样率、增益等）。

---

## 📁 创建的文件（可选保留）

这些文件不会影响系统，可以保留作为参考：

| 文件 | 说明 | 建议 |
|------|------|------|
| `main/audio/i2s_mic_full.cc` | ESP-SR 独立实现 | 📚 保留作为参考 |
| `main/audio/verify_esp_sr.cc` | 验证工具 | 📚 保留作为参考 |
| `main/audio/i2s_mic_simple.cc` | 简化版实现 | 📚 保留作为参考 |
| `*.md` 文档 | 使用说明 | 📚 保留作为参考 |

这些文件不会被编译（未在 CMakeLists.txt 中），不影响系统运行。

---

## 🎯 总结

### ✅ 您的系统状态

1. ✅ **已经有 ESP-SR** - 通过 `AfeAudioProcessor`
2. ✅ **已经有音频系统** - 完整的编解码器和服务
3. ✅ **已恢复正常** - 移除了冲突代码

### 🔧 改进降噪的建议

1. **修改 VAD 灵敏度**：`afe_audio_processor.cc` 中改为 `VAD_MODE_3`
2. **确认降噪启用**：检查日志是否加载了降噪模型
3. **硬件检查**：麦克风连接和位置
4. **编解码器配置**：检查音频参数设置

### 📖 参考文档

- 查看现有代码：`main/audio/processors/afe_audio_processor.cc`
- 参考实现：`main/audio/i2s_mic_full.cc`（不会被编译）
- ESP-SR 文档：`managed_components/espressif__esp-sr/README.md`

---

## 💻 重新编译

现在已经恢复，重新编译应该一切正常：

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

**抱歉造成了困扰！您的系统本来就有 ESP-SR，我们不需要再添加一个。** 🙏

如果需要改进降噪效果，可以调整 `AfeAudioProcessor` 的配置参数。



