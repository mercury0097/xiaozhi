# 🔍 如何检查 ESP-SR 是否被调用

## ✅ 已添加只读检查工具

我创建了一个**完全只读**的检查工具，它：
- ✅ **不启动任何硬件**
- ✅ **不影响原有系统**
- ✅ **只检查配置和状态**
- ✅ **给出查找建议**

---

## 🚀 立即测试

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 📊 您会看到什么

启动时，在原有日志**之前**会看到：

```
I (xxx) ESP-SR-Check: 
I (xxx) ESP-SR-Check: ╔════════════════════════════════════════╗
I (xxx) ESP-SR-Check: ║   ESP-SR 使用情况检查（只读模式）      ║
I (xxx) ESP-SR-Check: ╚════════════════════════════════════════╝

I (xxx) ESP-SR-Check: 📋 编译配置检查
I (xxx) ESP-SR-Check: ----------------------------------------
I (xxx) ESP-SR-Check: ✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用
I (xxx) ESP-SR-Check:    → 说明项目配置了音频处理器
I (xxx) ESP-SR-Check: ✅ PSRAM 支持: 已启用
I (xxx) ESP-SR-Check:    → PSRAM 大小: 8 MB
I (xxx) ESP-SR-Check: 📊 芯片信息:
I (xxx) ESP-SR-Check:    → 型号: esp32s3
I (xxx) ESP-SR-Check:    → CPU 频率: 240 MHz

I (xxx) ESP-SR-Check: 💾 内存状态
I (xxx) ESP-SR-Check: ----------------------------------------
I (xxx) ESP-SR-Check: 可用堆内存: XXX KB
I (xxx) ESP-SR-Check: 可用 PSRAM: XXX KB
I (xxx) ESP-SR-Check: ✅ 堆内存充足
I (xxx) ESP-SR-Check: ✅ PSRAM 充足

I (xxx) ESP-SR-Check: 🔍 ESP-SR 代码检查
I (xxx) ESP-SR-Check: ----------------------------------------
I (xxx) ESP-SR-Check: ✅ ESP-SR AFE 库: 已链接
I (xxx) ESP-SR-Check:    → esp_afe_handle_from_config 函数存在
I (xxx) ESP-SR-Check: ✅ ESP-SR 模型库: 已链接
I (xxx) ESP-SR-Check:    → esp_srmodel_init 函数存在

I (xxx) ESP-SR-Check: 💡 查看 ESP-SR 使用情况的方法
I (xxx) ESP-SR-Check: ----------------------------------------
I (xxx) ESP-SR-Check: 1️⃣ 查看 AfeAudioProcessor 日志:
I (xxx) ESP-SR-Check:    运行时查找关键词:
I (xxx) ESP-SR-Check:    - 'AfeAudioProcessor'
I (xxx) ESP-SR-Check:    - 'Audio communication task'
```

---

## 🎯 然后查看后续日志

检查完成后，继续查看**原有系统**的日志，查找：

### ✅ 如果 ESP-SR 在工作，应该看到：

```
I (xxx) AfeAudioProcessor: Initialize
I (xxx) AfeAudioProcessor: Audio communication task started
I (xxx) AfeAudioProcessor: feed size: 512 fetch size: 160
```

或者：

```
I (xxx) AfeAudioProcessor: Using ns model: nsnet3_ch1
```

### ❌ 如果没有看到这些

说明 ESP-SR 可能未被使用，或者使用的是其他音频处理方式。

---

## 🔍 查找技巧

### 方法 1：过滤日志

```bash
idf.py monitor | grep -i "afe\|audio"
```

### 方法 2：保存完整日志后搜索

```bash
idf.py monitor > system.log

# 在另一个终端搜索
grep -i "afeaudioprocessor" system.log
grep -i "esp-sr" system.log
grep -i "ns model" system.log
```

### 方法 3：查找关键词

启动后，在日志中按 Ctrl+F 搜索：
- `AfeAudioProcessor`
- `Audio communication`
- `feed size`
- `ns model`
- `nsnet`

---

## 📝 检查结果分析

### 场景 1：看到 ESP-SR 相关日志

```
✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用
✅ ESP-SR AFE 库: 已链接
✅ ESP-SR 模型库: 已链接

I (xxx) AfeAudioProcessor: Audio communication task started
```

**结论**：✅ ESP-SR 正在工作！

### 场景 2：配置启用，但没有运行日志

```
✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用
✅ ESP-SR AFE 库: 已链接

（但后续没有 AfeAudioProcessor 日志）
```

**可能原因**：
- 音频处理器未被启动
- 使用了其他音频处理方式
- 需要特定条件触发

### 场景 3：配置未启用

```
❌ CONFIG_USE_AUDIO_PROCESSOR: 未启用
```

**结论**：项目未配置使用音频处理器。

---

## 🛠️ 如果想要更详细的日志

### 方法 1：修改 AfeAudioProcessor（推荐）

编辑 `main/audio/processors/afe_audio_processor.cc`：

在 `Initialize()` 函数中添加：

```cpp
void AfeAudioProcessor::Initialize(...) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  AfeAudioProcessor 初始化");
    ESP_LOGI(TAG, "========================================");
    
    // ... 现有代码 ...
    
    afe_config_t* afe_config = afe_config_init(...);
    
    ESP_LOGI(TAG, "✅ AFE 配置:");
    ESP_LOGI(TAG, "   AEC: %s", afe_config->aec_init ? "启用" : "禁用");
    ESP_LOGI(TAG, "   NS:  %s", afe_config->ns_init ? "启用" : "禁用");
    ESP_LOGI(TAG, "   VAD: %s", afe_config->vad_init ? "启用" : "禁用");
    ESP_LOGI(TAG, "   VAD 模式: %d", afe_config->vad_mode);
    
    if (ns_model_name != nullptr) {
        ESP_LOGI(TAG, "✅ 使用降噪模型: %s", ns_model_name);
    } else {
        ESP_LOGI(TAG, "⚠️  未找到降噪模型");
    }
    
    // ... 其余代码 ...
}
```

### 方法 2：增加任务日志

在 `AudioProcessorTask()` 中添加：

```cpp
void AfeAudioProcessor::AudioProcessorTask() {
    ESP_LOGI(TAG, "🎤 音频处理任务开始");
    ESP_LOGI(TAG, "   Feed 大小: %d", feed_size);
    ESP_LOGI(TAG, "   Fetch 大小: %d", fetch_size);
    
    // ... 现有代码 ...
}
```

---

## 🎯 判断标准

### ✅ ESP-SR 正在工作的标志

1. 编译配置检查通过
2. ESP-SR 库已链接
3. 看到 `AfeAudioProcessor` 相关日志
4. 看到 `Audio communication task started`
5. （可选）看到降噪模型加载日志

### ⚠️ 可能未使用 ESP-SR

1. 配置未启用
2. 库未链接
3. 没有任何 AFE 相关日志
4. 只有编解码器日志，没有音频处理器日志

---

## 💡 常见情况

### 情况 1：语音识别时才启动

某些系统可能在以下情况才启动 AFE：
- 用户按下按钮
- 检测到唤醒词
- 进入特定模式

**解决**：尝试触发语音识别功能，然后查看日志。

### 情况 2：使用条件编译

可能根据板型不同，选择性使用 ESP-SR：

```cpp
#if CONFIG_USE_AUDIO_PROCESSOR
    // 使用 AfeAudioProcessor
#else
    // 使用 NoAudioProcessor
#endif
```

**查看**：检查您的板型配置。

---

## 🚫 如果不想保留检查工具

验证完成后，可以移除：

### 方法 1：注释掉调用（推荐）

编辑 `main/main.cc`：

```cpp
// check_esp_sr_usage();  // 注释掉这行
```

### 方法 2：完全移除

```bash
# 删除文件
rm main/audio/check_esp_sr.cc

# 编辑 CMakeLists.txt，移除这行：
# "audio/check_esp_sr.cc"
```

---

## 📊 检查清单

使用这个工具后，您应该能回答：

- [ ] ESP-SR 库是否已编译链接？
- [ ] 音频处理器配置是否启用？
- [ ] 运行时是否看到 AFE 相关日志？
- [ ] 是否加载了降噪模型？
- [ ] 内存是否充足？

---

## 🎉 总结

这个检查工具：
- ✅ **完全只读**，不启动任何硬件
- ✅ **不影响**原有系统运行
- ✅ **快速判断** ESP-SR 是否被使用
- ✅ **给出建议**，告诉您去哪里查看详细日志

**现在编译测试，看看结果！** 🚀

```bash
idf.py build flash monitor
```

