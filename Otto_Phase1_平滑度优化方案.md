# Otto Phase 1 - 平滑度优化方案

## 🎯 问题诊断

用户反馈：**动作太机械，不平滑，像是一个角度一个角度动一样**

## 🔍 根本原因

通过代码分析发现问题出在 `Oscillator` 类的采样周期设置：

```cpp
// oscillator.cc:20
sampling_period_ = 30;  // 30ms 采样周期
```

**影响**：
- 舵机每 **30ms** 才更新一次位置
- 对于 1-3° 的微小动作，视觉上非常明显地看到"顿挫感"
- 计算：30ms = 33.3 FPS，远低于流畅的 60 FPS 标准

## 🛠 解决方案

### 方案 1：降低采样周期（推荐）

修改 `main/boards/otto-robot/oscillator.cc`：

```cpp
// 第 20 行，从 30ms 改为 10ms
sampling_period_ = 10;  // 10ms = 100 FPS，足够平滑
```

**优点**：
- ✅ 100 FPS 更新率，肉眼看起来非常流畅
- ✅ 代码改动最小（仅 1 行）
- ✅ CPU 占用增加不多（从 33 FPS → 100 FPS）

**权衡**：
- ⚠️ CPU 占用增加约 3 倍（但对于 ESP32-S3 完全可承受）
- ⚠️ 功耗略微增加（但表达动作本身功耗占主导）

---

### 方案 2：自适应采样（高级）

根据动作幅度动态调整采样周期：

```cpp
// 在 OttoExpressiveController 中
void SetSamplingPeriod(int period) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        otto_->GetServo(i).SetSamplingPeriod(period);
    }
}

// Idle 呼吸：慢速采样（省电）
SetSamplingPeriod(20);  // 50 FPS

// Listening/Speaking：快速采样（平滑）
SetSamplingPeriod(10);  // 100 FPS
```

**优点**：
- ✅ 按需优化，平衡流畅度与功耗
- ✅ Idle 状态功耗更低

**缺点**：
- ⚠️ 需要修改 `Oscillator` 类添加 `SetSamplingPeriod()` 方法
- ⚠️ 代码复杂度增加

---

### 方案 3：使用硬件 PWM 插值（终极方案）

利用 ESP32 的 LEDC 淡入淡出功能：

```cpp
ledc_fade_func_install(0);
ledc_set_fade_with_time(mode, channel, target_duty, fade_time_ms);
ledc_fade_start(mode, channel, LEDC_FADE_NO_WAIT);
```

**优点**：
- ✅ 硬件级平滑，完全无顿挫
- ✅ CPU 占用极低（硬件处理）

**缺点**：
- ⚠️ 需要重构整个 `Oscillator` 类
- ⚠️ 工作量大，风险高

---

## 📋 推荐实施步骤

### 立即修复（5分钟）

修改 `oscillator.cc` 第 20 行：

```diff
- sampling_period_ = 30;
+ sampling_period_ = 10;  // 更平滑的舵机更新
```

**预期效果**：
- Idle 呼吸：从"机械抖动"变为"自然呼吸"
- Listening 姿态：从"分段运动"变为"流畅过渡"
- Speaking 点头：从"顿挫点头"变为"自然摆动"

---

### 进阶优化（可选）

如果 10ms 还不够平滑，可以进一步降低到 **5ms (200 FPS)**：

```cpp
sampling_period_ = 5;  // 极致平滑，但 CPU 占用翻倍
```

**性能对比**：

| 采样周期 | FPS  | 视觉效果 | CPU 占用（估算） |
|---------|------|---------|----------------|
| 30ms    | 33   | 明显顿挫 | 基准 (100%)    |
| 20ms    | 50   | 轻微顿挫 | ~150%          |
| 10ms    | 100  | 流畅     | ~300%          |
| 5ms     | 200  | 极致流畅 | ~600%          |

**建议**：先试 10ms，如果还不满意再改 5ms。

---

## 🧪 测试验证

修改后重新编译烧录，观察：

1. **Idle 呼吸**：手臂和脚踝应该像"真正在呼吸"，看不出明显的步进
2. **Listening 姿态**：切换到聆听姿态时应该一气呵成，无卡顿
3. **Speaking 点头**：点头动作应该像"人类自然点头"，不是"机械步进"

---

## 🔧 实施建议

**现在立即修复**：
1. 修改 `oscillator.cc:20`：`sampling_period_ = 10;`
2. 重新编译烧录
3. 测试观察效果

**如果 CPU 占用过高**（通过串口监视器 `I (xxx) cpu_start: cpu: XX%` 观察）：
- 考虑方案 2（自适应采样）
- 或者在 Idle 状态下暂时提高到 15ms

**如果还不够平滑**：
- 降低到 5ms
- 或考虑方案 3（硬件 PWM 插值，但需要大改）

---

## 📊 技术原理

### 为什么 30ms 会顿挫？

人眼的"流畅感知阈值"约为 **60 FPS (16.7ms)**：
- 30ms = 33 FPS：**低于阈值**，能明显看到离散的帧
- 10ms = 100 FPS：**高于阈值**，看起来连续流畅

对于 1° 的微小振幅：
- 30ms 间隔：每步 1°，看起来像"咔嗒咔嗒"
- 10ms 间隔：每步 0.33°，几乎看不出步进

### 为什么不用更小的周期？

- **5ms 以下**：边际收益递减，人眼已无法区分
- **功耗**：采样周期越小，CPU 唤醒越频繁，功耗越高
- **总线带宽**：I2C/SPI 总线可能成为瓶颈（本项目用 LEDC，无此问题）

---

## ✅ 总结

**立即行动**：改 `sampling_period_ = 10;`，重新编译。

**预期结果**：所有动作从"机械步进"变为"自然流畅"。

**后续优化**：如果需要进一步省电，可以实现方案 2（自适应采样）。




