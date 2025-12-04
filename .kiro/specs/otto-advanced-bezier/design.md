# Design Document: Otto Advanced Bezier Curves

## Overview

本设计将 Otto 机器人的所有基础动作升级为使用高级贝塞尔曲线缓动系统。通过为不同类型的动作选择合适的缓动曲线，使机器人动作更加自然、流畅、富有表现力。

核心思路：
- **保持现有 API 不变**，内部实现升级
- **为每种动作选择最合适的缓动类型**
- **利用现有的 `MoveServosWithEase()` 和 `EaseType` 基础设施**

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Motion Functions                          │
│  Walk, Turn, Jump, Bend, ShakeLeg, Swing, Moonwalker, etc.  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              MoveServosWithEase(time, target, ease_type)    │
│                    (Enhanced Core Function)                  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    GetEaseByType(t, type)                   │
│  EASE_LINEAR | EASE_IN | EASE_OUT | EASE_IN_OUT |          │
│  EASE_IN_BACK | EASE_OUT_BACK | EASE_OUT_BOUNCE            │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Servo Hardware Layer                      │
│              servo_[i].SetPosition(new_pos)                 │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. 缓动类型选择策略

| 动作类型 | 阶段 | 推荐缓动 | 原因 |
|---------|------|---------|------|
| **Walk** | 步行周期 | EASE_IN_OUT | 平滑的加减速，自然步态 |
| **Turn** | 开始 | EASE_IN | 逐渐加速，有动量感 |
| **Turn** | 结束 | EASE_OUT | 逐渐减速，稳定停止 |
| **Jump** | 上跳 | EASE_OUT | 爆发力，快速离地 |
| **Jump** | 下落 | EASE_IN | 模拟重力加速 |
| **Jump** | 落地 | EASE_OUT_BOUNCE | 弹跳吸收冲击 |
| **Bend** | 倾斜 | EASE_IN_OUT | 平滑重心转移 |
| **Bend** | 恢复 | EASE_OUT | 自然回弹 |
| **ShakeLeg** | 抬腿 | EASE_OUT_BACK | 夸张的抬起效果 |
| **ShakeLeg** | 抖动 | EASE_IN_OUT | 节奏感 |
| **Swing** | 摆动 | EASE_IN_OUT | 钟摆效果 |
| **UpDown** | 弹跳 | EASE_OUT_BOUNCE | 弹簧效果 |
| **Moonwalker** | 滑步 | EASE_IN_OUT | 平滑滑行 |
| **HandWave** | 抬手 | EASE_OUT_BACK | 热情的抬起 |
| **HandWave** | 挥动 | EASE_IN_OUT | 平滑摆动 |
| **Home** | 归位 | EASE_IN_OUT | 平稳回归 |

### 2. 修改的函数列表

需要修改的函数（将 `MoveServos` 替换为 `MoveServosWithEase`）：

1. **Jump()** - 分阶段使用不同缓动
2. **Bend()** - 使用 EASE_IN_OUT 和 EASE_OUT
3. **ShakeLeg()** - 使用 EASE_OUT_BACK 和 EASE_IN_OUT
4. **HandWave()** - 使用 EASE_OUT_BACK 和 EASE_IN_OUT
5. **HandWaveBoth()** - 使用 EASE_OUT_BACK 和 EASE_IN_OUT
6. **HandsUp()** - 使用 EASE_OUT_BACK
7. **HandsDown()** - 使用 EASE_IN_OUT
8. **Home()** - 使用 EASE_IN_OUT

### 3. 振荡器动作的处理

基于振荡器的动作（Walk, Turn, Swing, UpDown 等）使用 `OscillateServos()` 和 `Execute()`，这些函数内部使用正弦波生成运动。

对于这些动作，我们将：
- **保持振荡器核心逻辑不变**（正弦波是自然的周期运动）
- **升级 `Execute()` 中的归位阶段**，使用 `MoveServosWithEase()` 替代 `MoveServos()`

## Data Models

### EaseType 枚举（已存在）

```cpp
enum EaseType {
  EASE_LINEAR = 0,      // 线性
  EASE_IN_OUT = 1,      // 标准 S 型
  EASE_IN = 2,          // 慢启动
  EASE_OUT = 3,         // 快启动慢结束
  EASE_IN_BACK = 4,     // 回弹启动
  EASE_OUT_BACK = 5,    // 回弹结束
  EASE_OUT_BOUNCE = 6,  // 弹跳结束
};
```

### 动作阶段配置结构（新增，可选）

```cpp
// 可选：用于配置多阶段动作的缓动类型
struct MotionPhaseConfig {
  EaseType start_ease;   // 开始阶段缓动
  EaseType main_ease;    // 主要阶段缓动
  EaseType end_ease;     // 结束阶段缓动
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

根据预分析，本功能的大部分需求是关于内部实现选择（使用哪种缓动曲线），这些是主观的动作质量改进，最好通过人工观察验证。

只有一个可测试的正确性属性：

### Property 1: Home 函数完成后舵机位置准确性

*For any* 初始舵机位置配置，当 Home() 函数完成后，所有已连接的舵机位置应该精确等于目标归位位置（腿部/脚部为 90 度，手部为 HAND_HOME_POSITION 或其镜像）。

**Validates: Requirements 9.2**

## Error Handling

1. **无效舵机索引**：检查舵机索引范围，忽略未连接的舵机
2. **时间参数过小**：当 time <= 10ms 时，直接设置位置，跳过缓动计算
3. **缓动类型未知**：默认回退到 EASE_IN_OUT

## Testing Strategy

### 单元测试

由于本功能主要是动作质量改进，大部分验证需要通过人工观察。但可以进行以下单元测试：

1. **GetEaseByType 函数测试**
   - 验证各种 EaseType 在边界值（t=0, t=1）时返回正确值
   - 验证 EASE_LINEAR 返回线性值
   - 验证未知类型回退到默认行为

2. **Home 函数位置准确性测试**
   - 验证 Home() 完成后所有舵机位置正确

### 属性测试

使用 C++ 的属性测试库（如 RapidCheck）进行测试。

**Property 1 测试**：
- 生成随机的初始舵机位置（0-180 度范围）
- 调用 Home() 函数
- 验证所有舵机位置等于预期的归位位置

### 人工验证清单

以下动作需要通过人工观察验证动作质量：

- [ ] Walk - 步态平滑自然
- [ ] Turn - 转向有动量感
- [ ] Jump - 跳跃有弹性
- [ ] Bend - 弯腰平稳
- [ ] ShakeLeg - 抖腿有趣味
- [ ] Swing - 摇摆流畅
- [ ] UpDown - 弹跳有弹性
- [ ] Moonwalker - 滑步平滑
- [ ] HandWave - 挥手自然
- [ ] Home - 归位平稳
