# 🎯 VSCode中完整烧录（确保模型烧录成功）

## ⚠️ 问题分析

从日志看：
```
I (15543) 📦 Flash 中的模型数量: 0  ← 还是0
```

**原因**：可能使用了 "Flash (app-flash)" 而不是 "Flash (idf.py flash)"

- ❌ **app-flash** - 只烧录应用程序，**不包括model分区**
- ✅ **flash** - 烧录所有分区，**包括model分区**

---

## ✅ 正确的烧录步骤

### 方法1：VSCode ESP-IDF 插件（推荐）

#### 步骤：

1. **打开命令面板**
   - 按 `Cmd+Shift+P` (macOS)
   - 或 `Ctrl+Shift+P` (其他系统)

2. **输入并选择**：
   ```
   ESP-IDF: Flash your project
   ```
   **注意**：选择这个，不要选 "Build, Flash and Monitor"

3. **确认串口**：
   - 选择 `/dev/tty.usbmodem101`
   - 或在底部栏点击串口选择器

4. **等待烧录完成**
   - 看到 "Done" 或 "Flash complete"

5. **打开监控**：
   - 命令面板输入：`ESP-IDF: Monitor your device`
   - 或按底部栏的 Monitor 按钮

---

### 方法2：VSCode 终端命令（100%成功）

#### 步骤1：打开 ESP-IDF 终端

在 VSCode 中：
- 菜单：Terminal -> New Terminal
- 或快捷键：`` Ctrl+` ``

#### 步骤2：确认环境

在终端中运行：
```bash
idf.py --version
```

如果看到版本号（如 `ESP-IDF v5.5.1`），说明环境正常。

#### 步骤3：完整烧录（包含所有分区）

```bash
idf.py -p /dev/tty.usbmodem101 flash
```

**重要**：是 `flash`，不是 `app-flash`！

#### 步骤4：监控日志

```bash
idf.py -p /dev/tty.usbmodem101 monitor
```

或者一步到位：
```bash
idf.py -p /dev/tty.usbmodem101 flash monitor
```

---

### 方法3：使用脚本（备用）

我已经创建了 `完整烧录.sh` 脚本：

```bash
./完整烧录.sh
```

---

## 📊 验证成功的标志

### 烧录过程中应该看到：

```
Compressed 917504 bytes to xxxxx...
Writing at 0x00800000... (100 %)  ← 模型分区正在烧录
Wrote 917504 bytes (xxxxx compressed) at 0x00800000 in xx.x seconds
Hash of data verified.
```

**关键**：必须看到 `0x00800000` 这个地址的烧录！

### 启动后日志应该显示：

```
I (xxxx) AfeAudioProcessor: 📦 Flash 中的模型数量: 3  ← 变成3了！
I (xxxx) AfeAudioProcessor: ✅ ESP-SR 加载的模型数量: 3
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 0: wn9_nihaoxiaozhi_tts
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 1: vadnet1_medium  ← VAD恢复
I (xxxx) AfeAudioProcessor:    ESP-SR 模型 2: nsnet2          ← 降噪恢复
I (xxxx) AfeAudioProcessor: ✅ VAD 人声检测: VADNet1 (神经网络)
I (xxxx) AfeAudioProcessor: ✅ 使用 ESP-SR 神经网络降噪: nsnet2
I (xxxx) AFE: AFE Pipeline: [input] -> |AGC| -> |NS(nsnet2)| -> |VAD(VADNet1)| -> |WakeNet| -> [output]
```

**不再有警告**：
```
W (xxxx) ⚠️  未找到 vadnet1_medium   ← 应该消失
W (xxxx) ⚠️  未找到 NS 降噪模型       ← 应该消失
```

---

## 🔍 常见错误对比

### ❌ 错误做法（只烧录app）：

在 VSCode 中选择了：
- "Build and Flash (app-flash only)" ❌
- 或者运行：`idf.py app-flash` ❌

**结果**：
```
Flash 中的模型数量: 0  ← 模型没烧录
```

### ✅ 正确做法（烧录所有分区）：

在 VSCode 中选择：
- "ESP-IDF: Flash your project" ✅
- 或者运行：`idf.py flash` ✅

**结果**：
```
Flash 中的模型数量: 3  ← 成功！
```

---

## 🎯 快速检查清单

烧录前确认：

- [ ] 文件存在：`ls -lh build/srmodels/srmodels.bin`（应该是 896K）
- [ ] 使用正确命令：`idf.py flash`（不是 `app-flash`）
- [ ] 串口正确：`/dev/tty.usbmodem101`
- [ ] VSCode ESP-IDF 环境已激活

烧录后确认：

- [ ] 看到 `0x800000` 地址被写入
- [ ] 日志显示 "Flash 中的模型数量: 3"
- [ ] 日志显示 "VAD 人声检测: VADNet1 (神经网络)"
- [ ] 日志显示 "使用 ESP-SR 神经网络降噪: nsnet2"

---

## 💡 如果还是失败

### 尝试清空Flash后重新烧录：

```bash
# 警告：会清除所有数据！
idf.py -p /dev/tty.usbmodem101 erase-flash
idf.py -p /dev/tty.usbmodem101 flash
```

### 或者手动指定model分区：

```bash
# 只烧录model分区
python3 -m esptool -p /dev/tty.usbmodem101 write_flash 0x800000 build/srmodels/srmodels.bin
```

---

**现在试试在VSCode中运行 `idf.py flash` 吧！** 🚀

记得看烧录过程是否包含 `0x800000`！























