# Design Document: Palqiqi Performance Optimization

## Overview

本设计文档描述了 Palqiqi 机器人的性能优化方案，主要解决两个问题：
1. 说话时矢量眼睛动画帧率被降低（当前从 20Hz 降到 10Hz）
2. 语音识别响应速度慢

核心优化策略：
- **渲染优化**：使用增量渲染和脏区域检测，减少每帧渲染时间
- **任务调度优化**：调整任务优先级和 CPU 核心分配，避免音频和显示任务相互阻塞
- **异步处理**：将耗时操作移到后台，避免阻塞主循环

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 双核架构                         │
├─────────────────────────────────┬───────────────────────────────┤
│           CPU0                  │            CPU1               │
├─────────────────────────────────┼───────────────────────────────┤
│  ┌─────────────────────────┐    │  ┌─────────────────────────┐  │
│  │   LVGL Timer Task       │    │  │   Audio Output Task     │  │
│  │   (Priority: 5)         │    │  │   (Priority: 7)         │  │
│  │   - 矢量眼睛渲染        │    │  │   - I2S 音频输出        │  │
│  │   - UI 更新             │    │  │   - 播放队列处理        │  │
│  └─────────────────────────┘    │  └─────────────────────────┘  │
│                                 │                               │
│  ┌─────────────────────────┐    │  ┌─────────────────────────┐  │
│  │   Audio Input Task      │    │  │   Opus Codec Task       │  │
│  │   (Priority: 5)         │    │  │   (Priority: 6)         │  │
│  │   - 麦克风采集          │    │  │   - 音频编解码          │  │
│  │   - AFE 处理            │    │  │   - 重采样              │  │
│  └─────────────────────────┘    │  └─────────────────────────┘  │
│                                 │                               │
│  ┌─────────────────────────┐    │                               │
│  │   Main Event Loop       │    │                               │
│  │   (Priority: 3)         │    │                               │
│  │   - 状态管理            │    │                               │
│  │   - 协议处理            │    │                               │
│  └─────────────────────────┘    │                               │
└─────────────────────────────────┴───────────────────────────────┘
```

## Components and Interfaces

### 1. OptimizedVectorEyeDisplay

优化后的矢量眼睛显示类，继承自 `PalqiqiVectorEyeDisplay`。

```cpp
class OptimizedVectorEyeDisplay : public PalqiqiVectorEyeDisplay {
public:
    // 设置目标帧率 (10-30 FPS)
    void SetTargetFrameRate(uint8_t fps);
    
    // 获取当前实际帧率
    float GetActualFrameRate() const;
    
    // 启用/禁用自适应帧率
    void SetAdaptiveFrameRate(bool enable);
    
    // 获取上一帧渲染时间 (ms)
    uint32_t GetLastRenderTime() const;

private:
    // 脏区域标记
    bool dirty_ = true;
    
    // 帧率统计
    uint32_t frame_count_ = 0;
    uint32_t last_fps_update_ = 0;
    float actual_fps_ = 0.0f;
    
    // 渲染时间统计
    uint32_t last_render_time_ = 0;
    
    // 自适应帧率
    bool adaptive_fps_ = true;
    uint8_t target_fps_ = 20;
    uint8_t min_fps_ = 15;
};
```

### 2. PerformanceMonitor

性能监控组件，用于收集和报告系统性能指标。

```cpp
class PerformanceMonitor {
public:
    static PerformanceMonitor& GetInstance();
    
    // 记录帧渲染时间
    void RecordFrameTime(uint32_t time_ms);
    
    // 记录音频处理时间
    void RecordAudioProcessTime(uint32_t time_ms);
    
    // 获取统计信息
    struct Stats {
        float avg_frame_time;
        float avg_audio_time;
        uint32_t frame_drops;
        uint32_t audio_underruns;
    };
    Stats GetStats() const;
    
    // 检测 CPU 过载
    bool IsCpuOverloaded() const;
    
    // 打印诊断信息
    void PrintDiagnostics();
};
```

### 3. AudioServiceOptimizations

音频服务优化，减少处理延迟。

```cpp
// 在 AudioService 中添加的优化方法
class AudioService {
public:
    // 设置发送队列最大深度
    void SetMaxSendQueueDepth(size_t depth);
    
    // 获取当前队列深度
    size_t GetSendQueueDepth() const;
    size_t GetDecodeQueueDepth() const;
    size_t GetPlaybackQueueDepth() const;
    
    // 检查是否有积压
    bool HasBacklog() const;
};
```

## Data Models

### PerformanceConfig

```cpp
struct PerformanceConfig {
    // 显示配置
    uint8_t target_fps = 20;           // 目标帧率
    uint8_t min_fps = 15;              // 最低帧率
    uint32_t max_render_time_ms = 25;  // 最大渲染时间
    bool adaptive_fps = true;          // 自适应帧率
    
    // 音频配置
    size_t max_send_queue = 10;        // 发送队列最大深度
    size_t max_decode_queue = 10;      // 解码队列最大深度
    size_t max_playback_queue = 5;     // 播放队列最大深度
    
    // 任务配置
    uint8_t audio_output_priority = 7; // 音频输出优先级
    uint8_t audio_input_priority = 5;  // 音频输入优先级
    uint8_t lvgl_priority = 5;         // LVGL 任务优先级
};
```

### FrameStats

```cpp
struct FrameStats {
    uint32_t render_time_ms;      // 渲染耗时
    uint32_t update_time_ms;      // 更新耗时
    uint32_t total_time_ms;       // 总耗时
    bool was_skipped;             // 是否跳帧
    DeviceState device_state;     // 设备状态
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

Based on the prework analysis, the following properties can be verified through property-based testing:

### Property 1: Frame Rate Maintenance During Speaking

*For any* speaking session duration between 1-60 seconds, the measured frame rate SHALL remain at or above 15 FPS for at least 95% of the frames.

**Validates: Requirements 1.1**

### Property 2: State Transition Frame Continuity

*For any* state transition from Idle to Speaking, the time gap between consecutive frames SHALL not exceed 100ms (allowing for one frame drop at 10 FPS minimum).

**Validates: Requirements 1.3**

### Property 3: AFE Processing Latency

*For any* audio frame received after wake word detection, the AFE processing SHALL begin within 100ms of frame arrival.

**Validates: Requirements 2.1**

### Property 4: VAD Processing Time

*For any* audio frame processed by VAD, the processing time SHALL be less than 50ms.

**Validates: Requirements 2.2**

### Property 5: Send Queue Bounded Growth

*For any* sequence of N audio input frames, the send queue depth SHALL not exceed max(N/10, MAX_SEND_QUEUE_DEPTH).

**Validates: Requirements 2.4**

### Property 6: Render Time Bound

*For any* frame rendered by Vector_Eye_Display, the render time SHALL be less than 30ms.

**Validates: Requirements 3.3**

### Property 7: Frame Rate Configuration

*For any* target frame rate F between 10 and 30 FPS, setting the frame rate SHALL result in a timer interval of (1000/F) ± 5 ms.

**Validates: Requirements 4.1**

### Property 8: Runtime Configuration

*For any* configuration parameter change, the new value SHALL be reflected in system behavior within 100ms without requiring a reboot.

**Validates: Requirements 4.3**

## Safety Considerations

### 保持系统稳定性

**重要**：之前的 CPU 调整导致对话功能失效，因此本次优化采用保守策略：

1. **不修改现有任务的 CPU 核心分配**
   - 保持 audio_input 在 CPU0 (优先级 5)
   - 保持 audio_output 在 CPU1 (优先级 7)
   - 保持 opus_codec 在 CPU1 (优先级 6)

2. **只优化渲染逻辑**
   - 移除 Speaking 状态下的跳帧逻辑
   - 优化渲染算法减少 CPU 占用
   - 添加 taskYIELD() 让出 CPU 时间

3. **渐进式优化**
   - 先只移除跳帧，测试稳定性
   - 如果稳定，再添加渲染优化
   - 每步都验证对话功能正常

4. **回滚机制**
   - 保留原有代码作为备份
   - 添加配置开关可快速回滚

## Error Handling

### CPU Overload Detection

当检测到 CPU 过载时（连续 5 帧渲染时间超过 30ms）：
1. 记录警告日志，包含任务 CPU 使用率和队列深度
2. 如果启用自适应帧率，降低目标帧率到 min_fps
3. 暂停非关键动画（如随机表情变化）

### Audio Buffer Underrun

当音频播放队列为空时：
1. 记录警告日志
2. 增加 audio_underruns 计数器
3. 如果连续发生，提高音频任务优先级

### Queue Overflow

当队列达到最大深度时：
1. 丢弃最旧的数据包
2. 记录警告日志
3. 触发性能诊断输出

## Testing Strategy

### Unit Tests

1. **PerformanceMonitor 测试**
   - 测试统计计算的正确性
   - 测试 CPU 过载检测阈值

2. **配置测试**
   - 测试帧率设置范围验证
   - 测试队列深度设置

### Property-Based Tests

使用 ESP-IDF 的测试框架结合自定义属性测试：

1. **帧率属性测试** (Property 1, 6, 7)
   - 生成随机的设备状态序列
   - 验证帧率和渲染时间约束

2. **队列属性测试** (Property 5)
   - 生成随机的音频输入序列
   - 验证队列深度不会无限增长

3. **延迟属性测试** (Property 3, 4)
   - 生成随机的音频帧
   - 验证处理延迟在规定范围内

### Integration Tests

1. **端到端延迟测试**
   - 测量从唤醒词到系统响应的完整延迟

2. **并发压力测试**
   - 同时进行音频播放和动画渲染
   - 验证系统稳定性

### Test Framework

- 使用 ESP-IDF Unity 测试框架
- 属性测试使用简单的随机输入生成器
- 每个属性测试运行至少 100 次迭代
