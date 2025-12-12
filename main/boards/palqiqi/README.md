# Palqiqi 机器人开发板

帕奇奇（Palqiqi）是一款基于 ESP32-S3 的智能机器人开发板，具有以下特性：

## 硬件特性

- **主控芯片**: ESP32-S3
- **显示屏**: 240x240 ST7789 SPI LCD
- **音频**: I2S 数字麦克风 + 扬声器
- **舵机**: 6路舵机控制（双腿、双脚、双手）
- **电源管理**: 电池电量检测和充电状态监测

## 引脚配置

### 舵机引脚
- 左腿: GPIO17
- 右腿: GPIO39
- 左脚: GPIO18
- 右脚: GPIO38
- 左手: GPIO8
- 右手: GPIO12

### 音频引脚
- MIC_WS: GPIO4
- MIC_SCK: GPIO5
- MIC_DIN: GPIO6
- SPK_DOUT: GPIO7
- SPK_BCLK: GPIO15
- SPK_LRCK: GPIO16

### 显示屏引脚
- MOSI: GPIO10
- CLK: GPIO9
- DC: GPIO46
- RST: GPIO11
- CS: GPIO12
- 背光: GPIO3

## 编译方法

```bash
# 使用 release.py 脚本编译
python scripts/release.py palqiqi

# 或者手动配置编译
idf.py set-target esp32s3
idf.py menuconfig  # 选择 Xiaozhi Assistant -> Board Type -> Palqiqi Robot
idf.py build
idf.py flash monitor
```

## 功能特性

- 矢量眼睛显示（可切换为 GIF 表情模式）
- 贝塞尔曲线平滑运动控制
- MCP 协议机器人动作控制
- 空闲状态随机动作
- 舵机微调校准（NVS 持久化存储）

## 版本信息

当前版本: 1.0.0
