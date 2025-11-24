# 🔍 ESP-SR 验证指南

## 如何确认 ESP-SR 真的启用了？

我为您创建了一个完整的验证工具！

---

## 🚀 快速验证（3 步）

### 方法 1：在 main.cc 中调用验证函数

编辑 `main/main.cc`：

```cpp
#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "application.h"
#include "system_info.h"

#define TAG "main"

// 添加这两行
extern "C" void run_esp_sr_verification(void);
extern "C" void i2s_mic_full_start(void);

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 🔍 第 1 步：先验证 ESP-SR
    run_esp_sr_verification();
    
    // 等待 2 秒让您看到验证结果
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 🎤 第 2 步：启动麦克风
    i2s_mic_full_start();

    auto& app = Application::GetInstance();
    app.Start();
}
```

---

## 📊 预期输出（如果 ESP-SR 正常工作）

编译烧录后，串口会显示：

```
I (1234) ESP-SR-Verify: 
I (1234) ESP-SR-Verify: 🔍 开始 ESP-SR 完整验证...
I (1234) ESP-SR-Verify: 
I (1245) ESP-SR-Verify: 📚 ESP-SR 编译信息
I (1246) ESP-SR-Verify: ----------------------------------------
I (1247) ESP-SR-Verify: ✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用
I (1248) ESP-SR-Verify: ✅ PSRAM 支持: 已启用
I (1249) ESP-SR-Verify: 📊 芯片: esp32s3
I (1250) ESP-SR-Verify: 📊 CPU 频率: 240 MHz
I (1251) ESP-SR-Verify: ----------------------------------------

I (1252) ESP-SR-Verify: ========================================
I (1253) ESP-SR-Verify:   ESP-SR 组件验证
I (1254) ESP-SR-Verify: ========================================

I (1255) ESP-SR-Verify: 1️⃣ 检查 ESP-SR 版本信息
I (1256) ESP-SR-Verify:    IDF 版本: v5.5.1

I (1257) ESP-SR-Verify: 2️⃣ 检查 AFE 接口
I (1258) ESP-SR-Verify:    ✅ AFE 接口创建成功
I (1259) ESP-SR-Verify:    ✅ AFE 实例创建成功
I (1260) ESP-SR-Verify:    📊 Feed 块大小: 512
I (1261) ESP-SR-Verify:    📊 Fetch 块大小: 160

I (1262) ESP-SR-Verify: 3️⃣ 检查可用的模型
I (1263) ESP-SR-Verify:    ✅ 模型列表初始化成功
I (1264) ESP-SR-Verify:    ✅ 神经网络降噪模型: nsnet3_ch1
I (1265) ESP-SR-Verify:    ⚠️  未找到神经网络 VAD 模型（将使用 WEBRTC）
I (1266) ESP-SR-Verify:    ✅ 唤醒词模型: hilexin_wn5

I (1267) ESP-SR-Verify: 4️⃣ 内存状态检查
I (1268) ESP-SR-Verify:    可用堆内存: 285432 字节
I (1269) ESP-SR-Verify:    可用 PSRAM: 7864320 字节

I (1270) ESP-SR-Verify: ========================================
I (1271) ESP-SR-Verify:   ✅ ESP-SR 验证通过！
I (1272) ESP-SR-Verify:   可以正常使用音频处理功能
I (1273) ESP-SR-Verify: ========================================

I (3274) ESP-SR-Verify: 🎉 恭喜！ESP-SR 已正确配置并可以使用！
```

---

## ✅ 验证检查项

验证程序会检查：

### 1️⃣ 编译时检查
- ✅ CONFIG_USE_AUDIO_PROCESSOR 是否启用
- ✅ PSRAM 支持是否启用
- ✅ 目标芯片型号
- ✅ CPU 频率

### 2️⃣ AFE 接口检查
- ✅ AFE 接口是否可以创建
- ✅ AFE 实例是否可以初始化
- ✅ Feed/Fetch 块大小是否正确

### 3️⃣ 模型检查
- ✅ 神经网络降噪模型（nsnet）
- ✅ 神经网络 VAD 模型（vadnet）
- ✅ 唤醒词模型（wakenet）

### 4️⃣ 内存检查
- ✅ 堆内存是否充足（> 50KB）
- ✅ PSRAM 是否充足（> 100KB）

---

## 🔍 方法 2：查看编译日志

编译时查看是否链接了 ESP-SR 库：

```bash
idf.py build | grep "esp-sr"
```

**应该看到：**
```
[1234/2272] Linking CXX static library esp-idf/espressif__esp-sr/libesp-sr.a
[1235/2272] Building C object esp-idf/espressif__esp-sr/...
```

---

## 🔍 方法 3：查看链接的库文件

编译完成后检查：

```bash
ls -lh build/esp-idf/espressif__esp-sr/*.a
```

**应该看到：**
```
-rw-r--r--  1 user  staff   1.2M  libesp_audio_front_end.a
-rw-r--r--  1 user  staff   800K  libnsnet.a
-rw-r--r--  1 user  staff   500K  libvadnet.a
...
```

---

## 🔍 方法 4：运行时检查日志关键词

在 `i2s_mic_full_start()` 启动后，查看日志中是否有：

```
I (xxxx) I2SMicFull: ✅ ESP-SR AFE 初始化成功
I (xxxx) I2SMicFull:   Feed 块大小: 512
I (xxxx) I2SMicFull:   Fetch 块大小: 160
```

如果看到 **"使用神经网络降噪模型"**，说明启用了最强的降噪：
```
I (xxxx) I2SMicFull: 使用神经网络降噪模型: nsnet3_ch1
```

---

## ❌ 如果验证失败

### 问题 1：AFE 接口创建失败

```
❌ AFE 接口创建失败
```

**原因**：ESP-SR 库未正确链接

**解决**：
```bash
# 清理并重新编译
idf.py fullclean
idf.py build
```

### 问题 2：AFE 实例创建失败

```
❌ AFE 实例创建失败
```

**原因**：
- 内存不足
- PSRAM 未启用

**解决**：
```bash
idf.py menuconfig
# Component config → ESP32-specific → ☑ Support for external PSRAM
```

### 问题 3：CONFIG_USE_AUDIO_PROCESSOR 未启用

```
❌ CONFIG_USE_AUDIO_PROCESSOR: 未启用
```

**解决**：
```bash
idf.py menuconfig
# Component config → Audio HAL → ☑ Enable Audio Processor
```

或直接编辑 `sdkconfig`：
```
CONFIG_USE_AUDIO_PROCESSOR=y
```

---

## 📈 验证成功的标志

如果看到这些，说明 ESP-SR 完全正常：

| 检查项 | 成功标志 |
|-------|---------|
| **编译配置** | ✅ CONFIG_USE_AUDIO_PROCESSOR: 已启用 |
| **AFE 接口** | ✅ AFE 接口创建成功 |
| **AFE 实例** | ✅ AFE 实例创建成功 |
| **降噪模型** | ✅ 神经网络降噪模型: nsnet3_ch1 |
| **内存状态** | 可用堆内存 > 50KB，PSRAM > 100KB |
| **最终结果** | 🎉 恭喜！ESP-SR 已正确配置并可以使用！|

---

## 💻 完整测试流程

```bash
# 1. 编译
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
idf.py build

# 2. 检查是否链接 ESP-SR
idf.py build | grep "esp-sr"

# 3. 烧录
idf.py -p /dev/ttyUSB0 flash

# 4. 查看验证结果
idf.py -p /dev/ttyUSB0 monitor
```

---

## 🎯 总结

运行验证程序后，您会看到：

✅ **验证通过** = ESP-SR 完全启用，可以使用所有功能  
❌ **验证失败** = 需要检查配置，按照错误提示修复

**现在就编译试试吧！** 🚀

---

**提示**：验证完成后可以注释掉 `run_esp_sr_verification()` 这一行，或者保留它在每次启动时检查。

