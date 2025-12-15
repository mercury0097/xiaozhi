# Requirements Document

## Introduction

本需求文档描述了 Palqiqi 机器人的性能优化功能，主要解决两个问题：
1. 机器人说话时矢量眼睛动画帧率被降低导致动画不流畅
2. 语音识别响应速度慢

目标是实现说话时保持高帧率动画的同时不产生音频卡顿，并提升语音识别的响应速度。

## Glossary

- **Palqiqi_Robot**: 基于 ESP32-S3 的智能机器人，具有矢量眼睛显示、语音交互和舵机控制功能
- **Vector_Eye_Display**: 使用 LVGL canvas 实时绘制的矢量眼睛动画系统，当前实现在 `palqiqi_vector_eye_display.cc`
- **Audio_Service**: 负责音频输入/输出、编解码和语音处理的服务模块
- **AFE (Audio Front End)**: 音频前端处理，包括降噪、VAD、回声消除等
- **LVGL**: 轻量级图形库，用于 UI 渲染
- **Frame_Rate**: 动画每秒渲染的帧数，当前设置为 20Hz (50ms 间隔)，说话时降为 10Hz
- **Speaking_State**: 设备正在播放 AI 回复语音的状态 (kDeviceStateSpeaking)
- **Listening_State**: 设备正在监听用户语音输入的状态
- **Wake_Word_Detection**: 唤醒词检测功能，用于识别用户的唤醒指令

## Requirements

### Requirement 1

**User Story:** As a user, I want the robot's eye animation to remain smooth while it speaks, so that the robot appears more lifelike and engaging.

#### Acceptance Criteria

1. WHILE the Palqiqi_Robot is in Speaking_State, THE Vector_Eye_Display SHALL maintain a minimum frame rate of 15 FPS without audio stuttering
2. WHILE the Palqiqi_Robot is in Speaking_State, THE Audio_Service SHALL output audio without audible stuttering or gaps
3. WHEN the Palqiqi_Robot transitions from Idle_State to Speaking_State, THE Vector_Eye_Display SHALL continue rendering without visible frame drops
4. WHEN audio playback and animation rendering occur simultaneously, THE system SHALL use double buffering or asynchronous rendering to prevent blocking

### Requirement 2

**User Story:** As a user, I want faster voice recognition response, so that conversations with the robot feel more natural and responsive.

#### Acceptance Criteria

1. WHEN a user speaks after wake word detection, THE AFE SHALL begin processing audio within 100 milliseconds
2. WHEN the AFE processes audio input, THE system SHALL complete voice activity detection within 50 milliseconds per audio frame
3. WHILE Wake_Word_Detection is active, THE system SHALL respond to wake words within 500 milliseconds of utterance completion
4. WHEN audio input is received, THE Audio_Service SHALL encode and transmit audio packets without accumulating backlog in the send queue

### Requirement 3

**User Story:** As a developer, I want optimized task scheduling and CPU allocation, so that audio and display tasks do not interfere with each other.

#### Acceptance Criteria

1. WHEN the system initializes, THE audio output task SHALL run on CPU1 with priority 7 to ensure smooth playback
2. WHEN the system initializes, THE LVGL rendering task SHALL run on a separate CPU core from audio processing when possible
3. WHEN the Vector_Eye_Display renders a frame, THE rendering operation SHALL complete within 25 milliseconds to allow time for audio tasks
4. WHEN the Audio_Service processes audio, THE processing SHALL use non-blocking operations to prevent display starvation

### Requirement 4

**User Story:** As a developer, I want the rendering to be optimized for performance, so that the system can maintain high frame rates without excessive CPU usage.

#### Acceptance Criteria

1. WHEN the Vector_Eye_Display renders a frame, THE system SHALL use incremental rendering to update only changed regions
2. WHEN the eye animation updates, THE system SHALL cache computed values to avoid redundant calculations
3. WHEN the system detects high CPU load, THE Vector_Eye_Display SHALL adaptively reduce rendering complexity while maintaining visual quality
4. WHEN rendering completes, THE system SHALL yield CPU time to allow audio tasks to process
