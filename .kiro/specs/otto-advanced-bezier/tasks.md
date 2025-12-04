# Implementation Plan

- [ ] 1. 升级 Execute 函数的归位阶段
  - 修改 `Execute()` 函数（行 356），将归位阶段的 `MoveServos()` 替换为 `MoveServosWithEase()` 使用 EASE_IN_OUT
  - 这会影响所有基于振荡器的动作（Walk, Turn, Swing, UpDown, TiptoeSwing, Jitter, AscendingTurn, Moonwalker, Crusaito, Flapping）的归位平滑度
  - _Requirements: 1.1, 2.1, 6.1, 6.2, 6.3, 7.1, 7.2_

- [ ] 2. 升级 Jump 函数
  - 修改 `Jump()` 函数（行 408, 413），将 `MoveServos()` 替换为 `MoveServosWithEase()`
  - 上跳阶段使用 EASE_OUT（爆发力）
  - 下落阶段使用 EASE_IN（模拟重力）
  - 注意：`JumpBounce()` 已经使用高级曲线，无需修改
  - _Requirements: 3.1, 3.2_

- [ ] 3. 升级 Bend 函数
  - 修改 `Bend()` 函数（行 540-543），将 `MoveServos()` 替换为 `MoveServosWithEase()`
  - 倾斜阶段（bend1, bend2）使用 EASE_IN_OUT
  - 恢复阶段（homes）使用 EASE_OUT
  - _Requirements: 4.1, 4.2_

- [ ] 4. 升级 ShakeLeg 函数
  - 修改 `ShakeLeg()` 函数（行 586-593），将 `MoveServos()` 替换为 `MoveServosWithEase()`
  - 抬腿准备阶段（shake_leg1）使用 EASE_OUT_BACK（夸张效果）
  - 抬高阶段（shake_leg2）使用 EASE_IN_OUT
  - 抖动阶段（shake_leg3, shake_leg2 循环）使用 EASE_IN_OUT
  - 归位阶段（homes）使用 EASE_OUT
  - _Requirements: 5.1, 5.2, 5.3_

- [ ] 5. 升级 Home 函数
  - 修改 `Home()` 函数（行 385），将 `MoveServos()` 替换为 `MoveServosWithEase()` 使用 EASE_IN_OUT
  - _Requirements: 9.1, 9.2_

- [ ] 6. 升级手部动作函数
  - [ ] 6.1 升级 HandsUp 函数
    - 修改 `HandsUp()` 函数（行 825），将 `MoveServos()` 替换为 `MoveServosWithEase()`
    - 使用 EASE_OUT_BACK 抬手（热情的抬起效果）
    - _Requirements: 8.1_
  - [ ] 6.2 升级 HandsDown 函数
    - 修改 `HandsDown()` 函数（行 848），将 `MoveServos()` 替换为 `MoveServosWithEase()`
    - 使用 EASE_IN_OUT 放下（平稳下降）
    - _Requirements: 8.3_
  - [ ] 6.3 升级 HandWave 函数
    - 修改 `HandWave()` 函数（行 886-912），将所有 `MoveServos()` 替换为 `MoveServosWithEase()`
    - 抬手使用 EASE_OUT_BACK
    - 挥动使用 EASE_IN_OUT
    - 放下使用 EASE_IN_OUT
    - 注意：`HandWaveSmooth()` 已经使用高级曲线，无需修改
    - _Requirements: 8.1, 8.2, 8.3_
  - [ ] 6.4 升级 HandWaveBoth 函数
    - 修改 `HandWaveBoth()` 函数（行 939-956），将所有 `MoveServos()` 替换为 `MoveServosWithEase()`
    - 抬手使用 EASE_OUT_BACK
    - 挥动使用 EASE_IN_OUT
    - 放下使用 EASE_IN_OUT
    - _Requirements: 8.1, 8.2, 8.3_

- [ ] 7. 编写 Home 函数位置准确性属性测试
  - **Property 1: Home 函数完成后舵机位置准确性**
  - **Validates: Requirements 9.2**

- [ ] 8. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 9. 验证向后兼容性
  - 确认所有现有动作函数的 API 签名未改变
  - 确认 MoveServos 默认行为保持不变（仍可被其他代码调用）
  - _Requirements: 10.1, 10.2, 10.3_
