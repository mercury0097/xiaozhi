# VAD 模型快速恢复指南

## 🎯 问题症状

您的设备日志显示：
```
W (xxx) AfeAudioProcessor: ⚠️  未找到 vadnet1_medium
I (xxx) AFE: AFE Pipeline: [input] -> |NS(nsnet2)| -> |AGC(WebRTC)| -> |VAD(WebRTC)| -> [output]
```

**问题**：VAD 使用的是 WebRTC 模式，而不是神经网络模式（VADNet1）。

---

## ✅ 好消息

模型文件已经准备好了！
- ✅ 文件位置：`build/srmodels/srmodels.bin` (896KB)
- ✅ 包含模型：
  - nsnet2（降噪）
  - **vadnet1_medium**（VAD 神经网络）✨
  - wn9_nihaoxiaozhi_tts（唤醒词）

---

## 🚀 恢复步骤（超简单）

### 方法 1：使用恢复脚本（推荐）

**步骤 1**：连接您的 ESP32 设备到电脑

**步骤 2**：在 VSCode 终端中运行：

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
./恢复模型.sh
```

**如果提示找不到串口**，手动指定：
```bash
./恢复模型.sh /dev/cu.usbserial-XXXX
```

（将 `XXXX` 替换为您的实际串口号）

---

### 方法 2：在 VSCode 中一键烧录

**步骤 1**：点击 VSCode 底部状态栏的 **⚡ Flash** 按钮

或者点击 **⚡ Build, Flash and Monitor**

**这会自动烧录所有分区，包括模型分区！**

---

### 方法 3：手动烧录（如果上面两个都不行）

**步骤 1**：查找串口
```bash
ls /dev/cu.* | grep -E "(usbserial|wchusbserial|SLAB)"
```

**步骤 2**：烧录模型
```bash
esptool.py -p /dev/cu.usbserial-XXXX write_flash 0x800000 build/srmodels/srmodels.bin
```

（将 `/dev/cu.usbserial-XXXX` 替换为您的实际串口）

---

## 🎉 验证成功

设备重启后，日志应该显示：

```
I (xxx) AfeAudioProcessor: ✅ ESP-SR 加载的模型数量: 4
I (xxx) AfeAudioProcessor:    ESP-SR 模型 0: nsnet1
I (xxx) AfeAudioProcessor:    ESP-SR 模型 1: wn9_nihaoxiaozhi_tts
I (xxx) AfeAudioProcessor:    ESP-SR 模型 2: nsnet2
I (xxx) AfeAudioProcessor:    ESP-SR 模型 3: vadnet1_medium  ✅
I (xxx) AfeAudioProcessor: 🔍 指定 VAD 模型: vadnet1_medium (神经网络), 找到: vadnet1_medium  ✅
I (xxx) AfeAudioProcessor: ✅ VAD 人声检测: VADNet1 (神经网络)  ✅
I (xxx) AFE: AFE Pipeline: [input] -> |NS(nsnet2)| -> |AGC(WebRTC)| -> |VAD(VADNet1)| -> [output]  ✅
```

**关键标志**：
- ✅ `ESP-SR 模型 3: vadnet1_medium`
- ✅ `VAD 人声检测: VADNet1 (神经网络)`
- ✅ `|VAD(VADNet1)|`（而不是 WebRTC）

---

## ❓ 常见问题

### Q1: 为什么会丢失？

**原因**：删除 `build/` 目录后，模型文件被删除了。虽然重新编译会生成模型文件，但需要重新烧录到设备。

### Q2: 只烧录 app 可以吗？

**不行！** 必须烧录模型分区（0x800000），或者使用完整烧录。

### Q3: 如何避免以后再丢失？

**建议**：
1. 删除 build 后，使用 **⚡ Build, Flash and Monitor**（完整烧录）
2. 或者运行 `./恢复模型.sh`
3. 不要只烧录 app-flash

---

## 🔧 快速命令速查

```bash
# 查找串口
ls /dev/cu.* | grep -E "(usbserial|wchusbserial|SLAB)"

# 恢复模型（自动）
./恢复模型.sh

# 恢复模型（手动指定串口）
./恢复模型.sh /dev/cu.usbserial-XXXX

# 手动烧录
esptool.py -p /dev/cu.usbserial-XXXX write_flash 0x800000 build/srmodels/srmodels.bin

# 完整烧录（包含 app + 模型）
idf.py flash monitor
```

---

## 📊 对比：WebRTC vs VADNet1

| 特性 | WebRTC VAD | VADNet1（神经网络） |
|-----|-----------|------------------|
| 准确率 | 一般 | **高** ✨ |
| 抗噪能力 | 弱 | **强** ✨ |
| 误触发 | 较多 | **少** ✨ |
| CPU 占用 | 低 | 中等 |
| 推荐 | 临时使用 | **生产环境** ✨ |

**结论**：VADNet1 神经网络模式效果更好，强烈推荐恢复！

---

## ✅ 总结

**最简单的方法**：
1. 连接设备
2. VSCode 点击 **⚡ Flash**
3. 等待烧录完成
4. 重启设备
5. 查看日志确认 `VAD(VADNet1)`

或者运行：
```bash
./恢复模型.sh
```

**现在就试试吧！** 🚀




