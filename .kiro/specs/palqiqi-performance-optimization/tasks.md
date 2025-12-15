# Implementation Plan

## Phase 1: 移除跳帧逻辑（最小改动）

- [ ] 1. 移除 Speaking 状态下的跳帧逻辑
  - [x] 1.1 修改 `palqiqi_vector_eye_display.cc` 的 `OnUpdate()` 方法
    - 删除 `kDeviceStateSpeaking` 状态检查和 `skip_counter` 逻辑
    - 保留 `taskYIELD()` 调用以让出 CPU
    - _Requirements: 1.1, 1.3_
  - [x] 1.2 Write property test for frame rate maintenance
    - **Property 1: Frame Rate Maintenance During Speaking**
    - **Validates: Requirements 1.1**

- [ ] 2. Checkpoint - 验证基本功能
  - Ensure all tests pass, ask the user if questions arise.
  - 手动测试：确认对话功能正常工作
  - 手动测试：确认说话时动画流畅

## Phase 2: 添加性能监控

- [ ] 3. 添加帧率和渲染时间统计
  - [x] 3.1 在 `PalqiqiVectorEyeDisplay` 类中添加统计变量
    - 添加 `frame_count_`, `last_fps_update_`, `actual_fps_` 成员
    - 添加 `last_render_time_` 成员
    - _Requirements: 3.3, 4.4_
  - [x] 3.2 在 `OnUpdate()` 中记录渲染时间
    - 使用 `lv_tick_get()` 测量渲染耗时
    - 每秒计算一次实际帧率
    - 超过 30ms 时输出警告日志
    - _Requirements: 3.3_
  - [ ] 3.3 Write property test for render time bound
    - **Property 6: Render Time Bound**
    - **Validates: Requirements 3.3**

- [ ] 4. Checkpoint - 验证性能监控
  - Ensure all tests pass, ask the user if questions arise.
  - 查看日志确认帧率和渲染时间输出正确

## Phase 3: 可配置帧率

- [ ] 5. 添加帧率配置接口
  - [x] 5.1 添加 `SetTargetFrameRate()` 方法
    - 验证帧率范围 10-30 FPS
    - 更新 LVGL timer 周期
    - _Requirements: 4.1_
  - [x] 5.2 添加 `GetActualFrameRate()` 方法
    - 返回最近计算的实际帧率
    - _Requirements: 4.1_
  - [ ] 5.3 Write property test for frame rate configuration
    - **Property 7: Frame Rate Configuration**
    - **Validates: Requirements 4.1**

- [ ] 6. Checkpoint - 验证帧率配置
  - Ensure all tests pass, ask the user if questions arise.

## Phase 4: 自适应帧率（可选）

- [ ] 7. 实现自适应帧率
  - [ ] 7.1 添加 CPU 过载检测
    - 连续 5 帧渲染时间超过 30ms 视为过载
    - 过载时降低帧率到 15 FPS
    - 恢复正常后提升回目标帧率
    - _Requirements: 4.4_
  - [ ] 7.2 添加配置开关
    - `SetAdaptiveFrameRate(bool enable)` 方法
    - 默认启用自适应帧率
    - _Requirements: 4.3_
  - [ ] 7.3 Write property test for runtime configuration
    - **Property 8: Runtime Configuration**
    - **Validates: Requirements 4.3**

- [ ] 8. Final Checkpoint - 完整功能验证
  - Ensure all tests pass, ask the user if questions arise.
  - 验证所有功能正常工作
  - 验证对话功能不受影响
