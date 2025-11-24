# 小智 AI 机器人 - B端技术使用说明

## 📋 文档概述

**目标用户**：硬件开发者、IoT 工程师、技术集成商、企业技术团队  
**固件版本**：v2.0.3  
**更新日期**：2025-10-23  
**核心芯片**：ESP32-S3（同时支持 ESP32-C3/P4）  
**技术栈**：ESP-IDF 5.5.1 + ESP-SR 2.1.5 + LVGL 9.3.0 + FreeRTOS  

---

## 🎯 产品定位

小智 AI 是一款**开源的语音交互 AI 机器人解决方案**，基于 ESP32 系列芯片，通过 MCP（Model Context Protocol）协议实现大模型能力扩展，适用于：

- 🏭 **智能硬件产品开发**：快速构建语音交互产品原型
- 🏫 **教育科研项目**：AI + IoT 教学实验平台
- 🏠 **智能家居控制**：语音控制中枢设备
- 🤖 **机器人开发**：语音交互底层固件
- 🔬 **技术验证平台**：大模型 + 硬件集成方案验证

---

## 🏗️ 技术架构

### 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                      小智 AI 设备端                          │
├─────────────────────────────────────────────────────────────┤
│  应用层                                                      │
│  ├── Application (状态机管理)                               │
│  ├── UI Manager (LVGL 图形界面)                             │
│  ├── MCP Server (设备端工具注册)                            │
│  └── Pet System (电子宠物系统)                              │
├─────────────────────────────────────────────────────────────┤
│  音频处理层                                                   │
│  ├── ESP-SR (离线唤醒词识别 - WakeNet9)                      │
│  ├── AFE Audio Front-End                                     │
│  │   ├── NSNet2/3 神经网络降噪 (Deep Learning NS)            │
│  │   ├── VADNet1 人声活动检测 (Neural VAD)                   │
│  │   ├── WebRTC AEC 回声消除                                 │
│  │   ├── WebRTC AGC 自动增益控制                             │
│  │   └── SE 语音增强                                         │
│  ├── OPUS 编解码器 (16kbps 低带宽传输)                       │
│  └── I2S/PDM 麦克风/扬声器驱动                               │
├─────────────────────────────────────────────────────────────┤
│  通信层                                                       │
│  ├── WebSocket (推荐，JSON-RPC 2.0)                          │
│  ├── MQTT + UDP (混合协议，低延迟音频流)                     │
│  ├── WiFi (ESP32 内置)                                       │
│  └── 4G Cat.1 (ML307 模块，可选)                            │
├─────────────────────────────────────────────────────────────┤
│  硬件抽象层 (HAL)                                             │
│  ├── Board Config (70+ 开发板配置)                           │
│  ├── Codec Driver (音频芯片驱动)                             │
│  ├── Display Driver (LCD/OLED/AMOLED)                        │
│  ├── Camera Driver (可选)                                    │
│  └── Peripheral Driver (LED/Motor/GPIO)                      │
├─────────────────────────────────────────────────────────────┤
│  操作系统 / 驱动                                              │
│  └── ESP-IDF 5.5.1 + FreeRTOS                                │
└─────────────────────────────────────────────────────────────┘
                              ↕ 
┌─────────────────────────────────────────────────────────────┐
│                      云端服务 (可选)                          │
├─────────────────────────────────────────────────────────────┤
│  ├── xiaozhi.me 官方云服务 (免费 Qwen 实时模型)              │
│  ├── 自建服务器 (Python/Java/Go 开源实现)                    │
│  ├── ASR (语音识别): OpenAI Whisper / 讯飞 / Azure           │
│  ├── LLM (大模型): Qwen / DeepSeek / GPT-4 / Claude          │
│  ├── TTS (语音合成): Edge-TTS / Azure / 讯飞                 │
│  └── MCP 云端工具: 智能家居 / 知识搜索 / 邮件 / 日历         │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件版本（实际构建配置）

基于 `dependencies.lock` 确认的组件版本：

| 组件 | 版本 | 说明 |
|------|------|------|
| **ESP-IDF** | 5.5.1 | 官方开发框架（Espressif IoT Development Framework） |
| **ESP-SR** | 2.1.5 | 语音识别库（WakeNet9/VADNet1/NSNet2/3） |
| **LVGL** | 9.3.0 | 轻量级图形库（Light and Versatile Graphics Library） |
| **ESP LVGL Port** | 2.6.2 | LVGL 在 ESP32 上的适配层 |
| **ESP Opus** | 1.0.5 | OPUS 音频编解码库 |
| **ESP Opus Encoder** | 2.4.1 | OPUS 编码器优化版本 |
| **ESP32-Camera** | 2.1.3 | 摄像头驱动库（支持 OV2640/OV5640 等） |
| **ESP Codec Dev** | 1.4.0 | 音频编解码设备驱动框架 |
| **LED Strip** | 3.0.1 | WS2812/SK6812 LED 控制库 |
| **Button** | 4.1.4 | 按键驱动（支持单击/双击/长按） |
| **WiFi Connect** | 2.6.0 | WiFi 配网组件 |
| **ML307** | 3.3.6 | 4G Cat.1 模块驱动 |

**硬件支持**：70+ 开发板配置（详见 `main/boards/` 目录）

---

## 🔧 核心技术特性

### 1. 音频处理管线（Audio Processing Pipeline）

#### 1.1 神经网络降噪（NSNet）

**技术原理**：
- 基于深度卷积神经网络（CNN）的噪声抑制算法
- 在频域进行噪声掩码预测和抑制
- 支持 NSNet2 和 NSNet3 模型（模型大小约 500KB）
- 由 ESP-SR 组件提供（版本 2.1.5）

**性能指标**：
- CPU 占用：20-30%（ESP32-S3 @ 240MHz）
- 延迟：30-50ms
- SNR 提升：15-20dB（复杂噪声环境）
- 适用场景：音乐、电视、风扇、多人对话

**配置示例**：
```cpp
afe_config_t afe_config = {
    .ns_init = true,
    .afe_ns_mode = AFE_NS_MODE_NET,
    .ns_model_name = "nsnet3_ch1",  // 单麦克风
    // 多麦克风阵列可选：nsnet3_ch2, nsnet3_ch4
};
```

**备选方案**：WebRTC NS（传统信号处理，CPU 占用 5-10%，适合稳态噪声）

---

#### 1.2 人声活动检测（VAD）

**双 VAD 架构**：

**A. 唤醒词检测 VAD（WebRTC）**
```cpp
// 用于唤醒词前端检测，极低延迟
afe_config.vad_init = true;
afe_config.vad_mode = VAD_MODE_3;  // 0-4，越大越灵敏
afe_config.vad_min_noise_ms = 50;  // 噪声门限时间
```

**B. 语音上传 VAD（VADNet1 神经网络）**
```cpp
// 用于识别人声/非人声，精准过滤背景噪音
// 防止音乐误触发录音
afe_config.vadnet_init = true;
afe_config.vadnet_model = "vadnet1";
```

**优势**：
- 唤醒词检测：快速响应（<100ms）
- 上传过滤：防止音乐/电视误触发（误触发率 <0.1%）

---

#### 1.3 回声消除（AEC）

**技术实现**：ESP-SR AEC（基于 WebRTC AECM）

**应用场景**：
- Barge-in（打断）功能：播放 TTS 时仍可唤醒
- 播放音乐时识别语音
- 免提通话

**配置**：
```cpp
afe_config.aec_init = codec_->input_reference();  // 自动检测硬件支持
afe_config.aec_mode = AEC_MODE_VOIP_LOW_COST;    // 低资源占用模式
```

**硬件要求**：音频编解码器必须支持参考通道（Reference Channel）

---

#### 1.4 自动增益控制（AGC）

**当前配置（针对儿童语音优化）**：
```cpp
afe_config.agc_init = true;
afe_config.agc_mode = AFE_AGC_MODE_WEBRTC;
afe_config.agc_compression_gain_db = 15;  // 激进增益（建议 12-18）
afe_config.agc_target_level_dbfs = 3;     // 目标电平 -3dBFS
```

**效果**：
- 小声说话也能清晰识别
- 远场语音增强（1-3米有效距离）
- 适合儿童场景（音量变化大）

---

### 2. 离线语音唤醒（WakeNet9）

**技术架构**：
- CNN + LSTM 混合网络
- INT8 量化优化（模型大小约 1MB）
- 双核并行处理（CPU0: 应用，CPU1: 唤醒词检测）

**性能指标**：
| 指标 | 数值 |
|------|------|
| 唤醒率 | 95% |
| 误唤醒率 | <0.5 次/小时 |
| 响应时间 | <200ms |
| CPU 占用 | 30-40%（CPU1） |
| 功耗 | 约 80mA @ 240MHz |

**自定义唤醒词**：
- 使用 [xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) 在线生成
- 支持中文/英文拼音输入
- 可配置 1-3 个唤醒词

---

### 3. MCP 协议（Model Context Protocol）

**协议定义**：基于 JSON-RPC 2.0 的设备控制协议

**工作流程**：
```
1. initialize → 初始化 MCP 会话
2. tools/list → 获取设备支持的工具列表
3. tools/call → 调用具体工具
4. prompts/list → 获取提示词模板（可选）
```

**设备端工具注册示例**：
```cpp
#include "mcp_server.h"

void InitializeDeviceTools() {
    auto& mcp = McpServer::GetInstance();
    
    // 示例 1: 控制 LED
    mcp.AddTool(
        "self.led.set_color",           // 工具名称
        "设置 LED RGB 颜色",              // 描述
        PropertyList({                   // 参数定义
            Property("r", kPropertyTypeInteger, 0, 255),
            Property("g", kPropertyTypeInteger, 0, 255),
            Property("b", kPropertyTypeInteger, 0, 255)
        }),
        [this](const PropertyList& props) -> ReturnValue {
            int r = props["r"].value<int>();
            int g = props["g"].value<int>();
            int b = props["b"].value<int>();
            SetRGB(r, g, b);
            return true;  // 返回执行结果
        }
    );
    
    // 示例 2: 控制舵机
    mcp.AddTool(
        "self.servo.rotate",
        "旋转舵机到指定角度",
        PropertyList({
            Property("angle", kPropertyTypeInteger, 0, 180),
            Property("speed", kPropertyTypeInteger, 0, 1000)
        }),
        [this](const PropertyList& props) -> ReturnValue {
            int angle = props["angle"].value<int>();
            int speed = props["speed"].value<int>();
            ServoRotate(angle, speed);
            return true;
        }
    );
}
```

**后台调用示例**：
```json
// 请求
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "self.led.set_color",
    "arguments": {"r": 255, "g": 0, "b": 0}
  },
  "id": 1
}

// 响应
{
  "jsonrpc": "2.0",
  "result": {
    "content": [{"type": "text", "text": "true"}]
  },
  "id": 1
}
```

---

### 4. 通信协议

#### 4.1 WebSocket 协议（推荐）

**优势**：
- 全双工通信，低延迟
- JSON-RPC 2.0 格式，易于调试
- 支持 MCP 协议原生格式

**消息格式**：
```json
{
  "event": "audio_chunk",
  "opus_data": "<base64编码的OPUS音频>",
  "chunk_id": 123,
  "is_end": false
}
```

**详细文档**：[docs/websocket.md](docs/websocket.md)

---

#### 4.2 MQTT + UDP 混合协议

**设计思路**：
- MQTT：控制消息、文本数据（低频）
- UDP：音频流数据（高频，低延迟）

**优势**：
- 音频延迟更低（<50ms）
- 适合弱网络环境
- 支持组播（一对多）

**详细文档**：[docs/mqtt-udp.md](docs/mqtt-udp.md)

---

## 🛠️ 开发环境搭建

### 1. 环境要求

| 工具 | 版本 | 说明 |
|------|------|------|
| ESP-IDF | 5.5.1（推荐）或 5.3+ | 官方 SDK |
| Python | 3.8+ | ESP-IDF 依赖 |
| Cursor / VSCode | 最新版 | 推荐 IDE |
| CMake | 3.16+ | 构建工具 |
| Git | 2.x | 版本管理 |

**操作系统推荐**：
- ✅ **Linux（Ubuntu 20.04+）**：编译速度快，稳定性高
- ✅ **macOS**：开发体验好
- ⚠️ **Windows**：需要安装驱动，编译速度较慢

---

### 2. 快速开始

#### 2.1 克隆项目
```bash
git clone https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32
```

#### 2.2 安装 ESP-IDF
```bash
# Linux / macOS
curl -LO https://github.com/espressif/esp-idf/releases/download/v5.5.1/esp-idf-v5.5.1.zip
unzip esp-idf-v5.5.1.zip
cd esp-idf-v5.5.1
./install.sh esp32s3,esp32c3,esp32p4

# 激活环境
. ./export.sh

# 或者使用 ESP-IDF 扩展插件（VSCode/Cursor）
# 选择 SDK 版本：5.5.1
```

**注意**：
- 本项目基于 ESP-IDF 5.5.1 开发测试
- 理论上兼容 5.3.0 及以上版本
- 不建议使用低于 5.3.0 的版本

#### 2.3 配置开发板
```bash
# 查看支持的开发板列表
ls main/boards/

# 设置目标开发板（示例：立创实战派）
export XIAOZHI_BOARD=lichuang-dev

# 或者编辑 sdkconfig，设置：
# CONFIG_XIAOZHI_BOARD="lichuang-dev"
```

#### 2.4 编译烧录
```bash
# 配置（首次）
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash

# 查看日志
idf.py -p /dev/ttyUSB0 monitor
```

---

## 📦 自定义开发板适配

### 1. 创建开发板配置

```bash
# 创建开发板目录
mkdir -p main/boards/my-custom-board

# 创建配置文件
touch main/boards/my-custom-board/config.h
touch main/boards/my-custom-board/config.json
touch main/boards/my-custom-board/my_custom_board.cc
```

### 2. 配置文件示例（config.h）

```cpp
#pragma once
#define BOARD_NAME "My Custom Board"

// 音频配置
#define I2S_WS_PIN      42
#define I2S_SCK_PIN     41
#define I2S_DIN_PIN     2
#define I2S_DOUT_PIN    1

// 显示屏配置
#define LCD_CS_PIN      10
#define LCD_DC_PIN      11
#define LCD_RST_PIN     12
#define LCD_BL_PIN      13
#define LCD_WIDTH       240
#define LCD_HEIGHT      240

// LED 配置
#define LED_PIN         48
#define LED_NUM         1

// 电池检测
#define BATTERY_ADC_CHANNEL  ADC_CHANNEL_0
#define BATTERY_ADC_ATTEN    ADC_ATTEN_DB_11

// 功能开关
#define ENABLE_CAMERA   0
#define ENABLE_IMU      0
```

### 3. 注册开发板（my_custom_board.cc）

```cpp
#include "board.h"
#include "config.h"

namespace {

class MyCustomBoard : public Board {
 public:
  MyCustomBoard() : Board(BOARD_NAME) {}

  void Initialize() override {
    // 初始化硬件
    InitializeAudio();
    InitializeDisplay();
    InitializeLED();
  }

 private:
  void InitializeAudio() {
    // 配置 I2S 麦克风/扬声器
  }
  
  void InitializeDisplay() {
    // 配置 LCD
  }
  
  void InitializeLED() {
    // 配置 LED
  }
};

}  // namespace

DECLARE_BOARD(MyCustomBoard);
```

**详细教程**：[docs/custom-board.md](docs/custom-board.md)

---

## 🎨 UI 定制（LVGL）

### 1. 表情系统

**UI 框架**：LVGL 9.3.0（Light and Versatile Graphics Library）

**支持格式**：
- 静态图片：PNG / BMP
- 动态表情：GIF（使用 otto-emoji-gif-component v1.0.2）

**集成方式**：
```cpp
#include "display/display_manager.h"

auto& display = DisplayManager::GetInstance();

// 显示表情
display.ShowEmotion("happy");  // 预定义表情

// 自定义表情
display.ShowCustomEmotion("/spiffs/my_emoji.gif");
```

**在线生成工具**：[xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator)

---

### 2. 聊天界面

**特性**：
- 滚动消息列表
- 用户/AI 消息区分
- 表情联动
- 自适应字体大小

**修改样式**：
```cpp
// main/display/chat_screen.cc
lv_obj_set_style_bg_color(msg_box, lv_color_hex(0x1E90FF), 0);  // 背景色
lv_obj_set_style_text_color(msg_label, lv_color_hex(0xFFFFFF), 0);  // 字体色
```

---

## 🔌 外设扩展示例

### 1. 控制舵机（Otto 机器人）

```cpp
#include "peripherals/servo_controller.h"

ServoController servo;
servo.Initialize({GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7});

// 前进
servo.Forward(500);  // 速度参数

// 转弯
servo.TurnLeft(90);  // 角度

// 跳舞
servo.Dance();
```

### 2. 控制 WS2812 LED

```cpp
#include "peripherals/led_strip.h"

LedStrip led(LED_PIN, LED_NUM);
led.Initialize();

// 设置颜色
led.SetColor(0, 255, 0, 0);  // 索引, R, G, B

// 呼吸灯效果
led.BreathingEffect(lv_color_hex(0x00FF00), 2000);  // 颜色, 周期
```

### 3. 读取传感器（IMU）

```cpp
#include "peripherals/imu_sensor.h"

ImuSensor imu(I2C_NUM_0);
imu.Initialize();

// 读取加速度
float x, y, z;
imu.ReadAcceleration(&x, &y, &z);

// 检测摇晃
if (imu.IsShaking()) {
    ESP_LOGI(TAG, "设备被摇晃了！");
}
```

---

## 📊 性能优化指南

### 1. CPU 使用率优化

**双核任务分配策略**：
```cpp
// CPU0（APP_CPU）：应用逻辑
xTaskCreatePinnedToCore(
    application_task,
    "Application",
    8192,
    NULL,
    5,
    NULL,
    0  // CPU0
);

// CPU1（PRO_CPU）：音频处理
xTaskCreatePinnedToCore(
    audio_task,
    "Audio",
    8192,
    NULL,
    10,  // 更高优先级
    NULL,
    1  // CPU1
);
```

**监控 CPU 占用**：
```cpp
#include <esp_freertos_hooks.h>

void print_cpu_usage() {
    TaskStatus_t* tasks = (TaskStatus_t*)malloc(uxTaskGetNumberOfTasks() * sizeof(TaskStatus_t));
    uint32_t total_runtime;
    uxTaskGetSystemState(tasks, uxTaskGetNumberOfTasks(), &total_runtime);
    
    for (int i = 0; i < uxTaskGetNumberOfTasks(); i++) {
        float usage = (100.0f * tasks[i].ulRunTimeCounter) / total_runtime;
        ESP_LOGI(TAG, "Task %s: %.2f%%", tasks[i].pcTaskName, usage);
    }
    free(tasks);
}
```

---

### 2. 内存优化

**PSRAM 使用**（ESP32-S3）：
```cpp
// 分配大缓冲区到 PSRAM
void* buffer = heap_caps_malloc(1024 * 1024, MALLOC_CAP_SPIRAM);

// 检查内存使用
ESP_LOGI(TAG, "Free heap: %d KB", esp_get_free_heap_size() / 1024);
ESP_LOGI(TAG, "Free PSRAM: %d KB", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
```

**内存泄漏检测**：
```bash
# 启用内存泄漏检测
idf.py menuconfig
# Component config → Heap memory debugging → Enable

# 编译运行后查看
idf.py monitor
# 设备重启时会自动打印泄漏信息
```

---

### 3. 降低功耗

**动态频率调整**：
```cpp
#include <esp_pm.h>

esp_pm_config_t pm_config = {
    .max_freq_mhz = 240,  // 最高频率
    .min_freq_mhz = 80,   // 空闲时降频
    .light_sleep_enable = true
};
esp_pm_configure(&pm_config);
```

**WiFi 省电模式**：
```cpp
esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // 最小省电模式
```

---

## 🧪 测试与调试

### 1. 串口日志级别

```cpp
// 全局日志级别
esp_log_level_set("*", ESP_LOG_INFO);

// 模块单独设置
esp_log_level_set("Audio", ESP_LOG_DEBUG);
esp_log_level_set("Network", ESP_LOG_WARN);
```

### 2. 音频质量测试

**录制音频保存到 SD 卡**：
```cpp
#include <stdio.h>

FILE* f = fopen("/sdcard/recording.pcm", "wb");
fwrite(audio_data, sizeof(int16_t), sample_count, f);
fclose(f);

// 使用 Audacity 等工具播放：
// 格式：16-bit PCM, 16000 Hz, Mono
```

### 3. 网络抓包

```bash
# 安装 Wireshark
sudo apt install wireshark

# 过滤 MQTT 流量
mqtt

# 过滤 WebSocket 流量
websocket
```

---

## 📈 生产部署建议

### 1. OTA 固件升级

**启用 OTA**：
```cpp
#include <esp_ota_ops.h>

void ota_upgrade(const char* url) {
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = server_cert,
    };
    esp_https_ota(&config);
}
```

**分区表配置**（partitions/v2/partitions.csv）：
```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x6000
phy_init, data, phy,     0xf000,  0x1000
factory,  app,  factory, 0x10000, 0x200000
ota_0,    app,  ota_0,   0x210000, 0x200000
ota_1,    app,  ota_1,   0x410000, 0x200000
spiffs,   data, spiffs,  0x610000, 0x100000
```

---

### 2. 大规模生产烧录

**生成批量烧录文件**：
```bash
idf.py build
esptool.py --chip esp32s3 merge_bin \
    -o merged_firmware.bin \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 8MB \
    0x0000 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x10000 build/xiaozhi.bin
```

**烧录命令**：
```bash
esptool.py -p /dev/ttyUSB0 -b 921600 write_flash 0x0 merged_firmware.bin
```

---

### 3. 质量检测脚本

```python
# test_device.py
import serial
import time

def test_device(port):
    ser = serial.Serial(port, 115200, timeout=5)
    time.sleep(2)
    
    # 检查启动日志
    logs = ser.read(1000).decode('utf-8', errors='ignore')
    
    tests = {
        "Boot": "Application started" in logs,
        "WiFi": "WiFi connected" in logs,
        "Audio": "Audio initialized" in logs,
        "Display": "Display ready" in logs,
    }
    
    print(f"Device on {port}:")
    for name, result in tests.items():
        print(f"  {name}: {'✓ PASS' if result else '✗ FAIL'}")
    
    ser.close()
    return all(tests.values())

if __name__ == "__main__":
    test_device("/dev/ttyUSB0")
```

---

## 🔒 安全性考虑

### 1. WiFi 安全

**使用 WPA3**：
```cpp
wifi_config_t wifi_config = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .threshold.authmode = WIFI_AUTH_WPA3_PSK,  // WPA3
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    },
};
```

### 2. 数据加密

**TLS/SSL 连接**：
```cpp
esp_http_client_config_t config = {
    .url = "https://api.xiaozhi.me",
    .cert_pem = server_cert,
    .client_cert_pem = client_cert,  // 客户端证书（可选）
    .client_key_pem = client_key,
};
```

### 3. 固件签名

```bash
# 生成签名密钥
espsecure.py generate_signing_key secure_boot_key.pem

# 启用安全启动
idf.py menuconfig
# Security features → Enable secure boot version 2
```

---

## 📚 API 参考

### 1. 核心 API

#### Application
```cpp
class Application {
 public:
  static Application& GetInstance();
  void Start();
  void Stop();
  
  // 状态管理
  void SetState(AppState state);
  AppState GetState() const;
};
```

#### McpServer
```cpp
class McpServer {
 public:
  static McpServer& GetInstance();
  
  // 注册工具
  void AddTool(
      const std::string& name,
      const std::string& description,
      const PropertyList& properties,
      std::function<ReturnValue(const PropertyList&)> callback
  );
  
  // 注册提示词
  void AddPrompt(
      const std::string& name,
      const std::string& description,
      const std::string& content
  );
};
```

#### AudioProcessor
```cpp
class AudioProcessor {
 public:
  virtual bool Initialize(const AudioConfig& config) = 0;
  virtual void Process(int16_t* data, size_t samples) = 0;
  virtual void SetCallback(AudioCallback callback) = 0;
};
```

---

## 🆘 故障排查

### 常见问题

**Q1: 编译失败 - "esp_sr.h: No such file"**
```bash
# 更新子模块
git submodule update --init --recursive

# 清理重新编译
idf.py fullclean && idf.py build
```

**Q2: 烧录失败 - "A fatal error occurred: Failed to connect"**
```bash
# 检查串口权限
sudo usermod -a -G dialout $USER
# 重新登录

# 尝试更低波特率
esptool.py -p /dev/ttyUSB0 -b 115200 flash
```

**Q3: 音频噪声过大**
```cpp
// 调整 AGC 增益
afe_config.agc_compression_gain_db = 12;  // 降低增益

// 或禁用 AGC
afe_config.agc_init = false;
```

**Q4: CPU 过载导致重启**
```cpp
// 降低音频处理优先级
xTaskCreatePinnedToCore(..., priority: 5, ...);  // 从 10 降到 5

// 关闭不必要的功能
afe_config.se_init = false;  // 关闭语音增强
```

---

## 📖 技术文档索引

- [自定义开发板指南](docs/custom-board.md)
- [MCP 协议详解](docs/mcp-protocol.md)
- [MCP 使用说明](docs/mcp-usage.md)
- [WebSocket 协议](docs/websocket.md)
- [MQTT+UDP 协议](docs/mqtt-udp.md)
- [音频算法指南](AUDIO_ALGORITHMS_GUIDE.md)
- [I2S 麦克风集成](I2S_MIC_QUICK_START.md)
- [降噪优化指南](NOISE_REDUCTION_IMPROVEMENT.md)

---

## 🌐 相关资源

### 开源服务器
- [Python 服务器](https://github.com/xinnan-tech/xiaozhi-esp32-server)
- [Java 服务器](https://github.com/joey-zhou/xiaozhi-esp32-server-java)
- [Golang 服务器](https://github.com/AnimeAIChat/xiaozhi-server-go)

### 第三方客户端
- [Python 客户端](https://github.com/huangjunsen0406/py-xiaozhi)
- [Android 客户端](https://github.com/TOM88812/xiaozhi-android-client)
- [Linux 客户端](http://github.com/100askTeam/xiaozhi-linux)

### 工具
- [自定义 Assets 生成器](https://github.com/78/xiaozhi-assets-generator)

---

## 📞 技术支持

- **GitHub Issues**：https://github.com/78/xiaozhi-esp32/issues
- **QQ 群**：1011329060
- **飞书文档**：[小智 AI 机器人百科全书](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb)
- **官方网站**：https://xiaozhi.me

---

## 📄 许可证

本项目采用 **MIT 许可证**，允许免费用于商业和个人用途。

```
MIT License

Copyright (c) 2024 虾哥（78）

Permission is hereby granted, free of charge, to any person obtaining a copy...
```

---

## 🎯 商业合作

如需技术支持、定制开发、批量采购，请联系：
- 邮箱：support@xiaozhi.me
- QQ 群：1011329060

---

## 📋 技术规格总结

| 项目 | 规格 |
|------|------|
| **核心芯片** | ESP32-S3（主要）、ESP32-C3、ESP32-P4 |
| **固件版本** | v2.0.3 |
| **ESP-IDF** | 5.5.1 |
| **ESP-SR** | 2.1.5（WakeNet9 + VADNet1 + NSNet2/3） |
| **LVGL** | 9.3.0 |
| **音频编解码** | OPUS 1.0.5 + ESP Opus Encoder 2.4.1 |
| **通信协议** | WebSocket（JSON-RPC 2.0）/ MQTT+UDP |
| **WiFi** | 2.4GHz（ESP32-S3/C3）/ 2.4GHz+5GHz（部分型号） |
| **4G 模块** | ML307 Cat.1（可选） |
| **摄像头** | ESP32-Camera 2.1.3（可选） |
| **显示屏** | 支持 SPI/RGB/I80/QSPI 接口 |
| **触摸屏** | 支持 GT911/FT5x06/CST816S 等 |
| **内存** | 最低 512KB SRAM + 2MB PSRAM（推荐） |
| **闪存** | 最低 8MB（推荐 16MB） |

---

**更新日期**：2025-10-23  
**文档版本**：2.1  
**适用固件版本**：v2.0.3+  
**测试环境**：ESP32-S3 + ESP-IDF 5.5.1

