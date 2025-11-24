# ⚡ 快速测试指南

## ✅ 已完成修改

`main/main.cc` 已经添加了 ESP-SR 验证和麦克风启动代码！

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

## 📊 预期输出

现在您会在日志中看到：

```
I (xxx) main: 
I (xxx) main: 🎤 开始启动 ESP-SR 音频系统...

I (xxx) ESP-SR-Verify: 🔍 开始 ESP-SR 完整验证...

I (xxx) ESP-SR-Verify: ✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用
I (xxx) ESP-SR-Verify: ✅ PSRAM 支持: 已启用
I (xxx) ESP-SR-Verify: 📊 芯片: esp32s3
I (xxx) ESP-SR-Verify: 📊 CPU 频率: 240 MHz

I (xxx) ESP-SR-Verify: ✅ AFE 接口创建成功
I (xxx) ESP-SR-Verify: ✅ AFE 实例创建成功
I (xxx) ESP-SR-Verify: 📊 Feed 块大小: 512
I (xxx) ESP-SR-Verify: 📊 Fetch 块大小: 160

I (xxx) ESP-SR-Verify: ✅ 神经网络降噪模型: nsnet3_ch1  ← 重点！
I (xxx) ESP-SR-Verify: ✅ 唤醒词模型: hilexin_wn5

I (xxx) ESP-SR-Verify: ✅ ESP-SR 验证通过！
I (xxx) ESP-SR-Verify: 🎉 恭喜！ESP-SR 已正确配置并可以使用！

I (xxx) I2SMicFull: ==========================================
I (xxx) I2SMicFull:   I2S 麦克风 + ESP-SR AFE 专业降噪
I (xxx) I2SMicFull:   采样率: 16000 Hz
I (xxx) I2SMicFull:   引脚: WS=42, SCK=41, DIN=2
I (xxx) I2SMicFull: ==========================================
I (xxx) I2SMicFull: ✓ I2S 初始化成功
I (xxx) I2SMicFull: ✓ ESP-SR AFE 初始化成功
I (xxx) I2SMicFull: ✓ ESP-SR AFE 音频处理已启动

I (xxx) main: ✅ ESP-SR 音频系统启动完成

I (xxx) I2SMicFull: … Silence           ← 安静时
I (xxx) I2SMicFull: 🎤 Voice detected   ← 说话时
```

---

## ⚠️ 重要：检查引脚配置

您的 Otto 机器人使用了以下 GPIO（舵机控制）：
- GPIO 17, 39, 18, 38, 8, 12

**I2S 引脚不能与这些冲突！**

当前默认配置：
```cpp
#define I2S_WS_PIN      42    // Word Select
#define I2S_SCK_PIN     41    // Serial Clock
#define I2S_DIN_PIN     2     // Data In
```

✅ **这些引脚不冲突，可以直接使用！**

如果您的麦克风引脚不同，请编辑：
```bash
vim main/audio/i2s_mic_full.cc
# 修改第 29-31 行的引脚定义
```

---

## 🎯 验证关键点

看到以下信息说明成功：

### 1️⃣ ESP-SR 验证通过
```
✅ ESP-SR 验证通过！
```

### 2️⃣ AFE 初始化成功
```
✓ ESP-SR AFE 初始化成功
```

### 3️⃣ 神经网络降噪（最重要！）
```
✅ 神经网络降噪模型: nsnet3_ch1
```

如果看到这个，说明您使用的是**最强的降噪算法**！

---

## 🐛 如果出现问题

### 问题 1：AFE 初始化失败

```
❌ AFE 实例创建失败
```

**原因**：内存不足或 PSRAM 未启用

**您的系统**：✅ 已有 8MB PSRAM，应该没问题

### 问题 2：I2S 初始化失败

```
❌ I2S 初始化失败
```

**原因**：引脚冲突或麦克风未连接

**检查**：
1. 麦克风是否连接到正确的引脚
2. 引脚是否与舵机冲突
3. 麦克风供电是否正常（3.3V）

### 问题 3：没有看到任何输出

**原因**：编译时没有包含新代码

**解决**：
```bash
idf.py fullclean
idf.py build
```

---

## 📝 修改后的文件

| 文件 | 状态 | 说明 |
|------|------|------|
| ✅ `main/main.cc` | **已修改** | 添加了验证和启动代码 |
| ✅ `main/CMakeLists.txt` | 已修改 | 包含了新文件 |
| ✅ `main/audio/i2s_mic_full.cc` | 已创建 | ESP-SR 完整版 |
| ✅ `main/audio/verify_esp_sr.cc` | 已创建 | 验证工具 |

---

## 🎉 成功标志

如果看到这些日志，说明一切正常：

```
🎉 恭喜！ESP-SR 已正确配置并可以使用！
✓ ESP-SR AFE 音频处理已启动
✅ ESP-SR 音频系统启动完成
```

然后每隔 1 秒会输出：
- `… Silence` - 环境安静
- `🎤 Voice detected` - 检测到语音

---

## 🔧 可选：验证后禁用日志

如果验证通过，不想每次都看到验证信息，可以注释掉：

编辑 `main/main.cc`：
```cpp
// run_esp_sr_verification();  // 注释掉这行
```

但建议**先验证成功**再注释！

---

**现在就编译测试吧！** 🚀

```bash
idf.py build flash monitor
```



