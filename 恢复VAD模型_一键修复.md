# 🔧 VAD 模型丢失 - 一键修复

## 🔍 问题诊断

你的日志显示：
```
⚠️  未找到 vadnet1_medium，尝试 vadnet1...
⚠️  未找到任何 VADNet 模型
📦 Flash 中的模型数量: 3  ← 只有唤醒词和降噪，缺少 VAD
AFE Pipeline: |VAD(WebRTC)| ← 回退到 WebRTC VAD
```

**原因**：`model` 分区没有正确烧录 VADNet1 模型

---

## ✅ 快速修复（推荐）

### 方法1：VSCode 一键完整烧录 ⭐

1. **打开 VSCode**

2. **按 `Cmd+Shift+P`**，输入：
   ```
   ESP-IDF: Flash your project
   ```

3. **回车**，等待烧录完成

4. **验证**：在日志中看到 `0x800000` 被烧录：
   ```
   Writing at 0x00800000... (100 %)  ← 必须有这一行！
   Wrote 917504 bytes at 0x00800000
   ```

5. **重启设备**，检查日志：
   ```
   ✅ 成功标志：
   📦 Flash 中的模型数量: 3
   ✅ VAD 人声检测: vadnet1_medium (神经网络)
   AFE Pipeline: |NS(nsnet2)| -> |VAD(vadnet1_medium)|
   ```

---

### 方法2：终端命令（备用）

如果 VSCode 烧录有问题，用终端：

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main

# 完整烧录（包括 model 分区）
idf.py -p /dev/tty.usbmodem101 flash
```

**重要**：必须用 `flash`，不要用 `app-flash`！

---

## 🚫 常见错误

### ❌ 错误1：只烧录 app

```bash
idf.py app-flash  ❌ 不会烧录 model 分区
```

**后果**：模型分区保持旧数据或为空

### ❌ 错误2：VSCode 选错选项

在 VSCode 底部点了：
- "Build and Flash (app-flash only)" ❌

**应该选择**：
- `Cmd+Shift+P` → "ESP-IDF: Flash your project" ✅

---

## 📊 验证成功的标志

### 烧录时的日志：

```
Wrote 917504 bytes (xxxxx compressed) at 0x00000000 in x.x seconds  ← bootloader
Wrote 3072 bytes (xxxxx compressed) at 0x00008000 in x.x seconds    ← partition table
Wrote 1048576 bytes (xxxxx compressed) at 0x00010000 in x.x seconds ← app
Wrote 917504 bytes (xxxxx compressed) at 0x00800000 in x.x seconds  ← model ⭐ 必须有！
```

### 启动后的日志：

```
I (xxxx) AfeAudioProcessor: 📦 Flash 中的模型数量: 3
I (xxxx) AfeAudioProcessor:    Flash 模型 0: wn9_nihaoxiaozhi_tts
I (xxxx) AfeAudioProcessor: ✅ ESP-SR 加载的模型数量: 3
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 0: wn9_nihaoxiaozhi_tts
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 1: vadnet1_medium  ← VAD 回来了！
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 2: nsnet2
I (xxxx) AfeAudioProcessor: ✅ VAD 人声检测: vadnet1_medium (神经网络)  ← 不再是 WebRTC
I (xxxx) AFE: AFE Pipeline: [input] -> |NS(nsnet2)| -> |AGC(WebRTC)| -> |VAD(vadnet1_medium)| -> [output]
```

**不应该再有警告**：
- ~~`⚠️ 未找到 vadnet1_medium`~~ ❌
- ~~`⚠️ 未找到任何 VADNet 模型`~~ ❌

---

## 🎯 立即执行

现在就在 VSCode 中：

1. **按 `Cmd+Shift+P`**
2. **输入**：`ESP-IDF: Flash your project`
3. **回车**
4. **等待完成**（大约 1-2 分钟）
5. **重启设备**
6. **监控日志**：看到 `vadnet1_medium` 即成功！

---

## 💡 为什么会丢失？

可能的原因：
1. 之前用了 `app-flash` 而不是 `flash`
2. VSCode 选错了烧录选项
3. 手动烧录时漏掉了 model 分区
4. 使用了错误的分区表配置

**解决方案**：始终使用完整烧录 `idf.py flash`

---

**现在就去烧录吧！** 🚀





