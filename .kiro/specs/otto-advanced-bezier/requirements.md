# Requirements Document

## Introduction

本功能旨在将 Otto 机器人的所有基础动作升级为使用高级贝塞尔曲线，使动作更加自然、流畅，具有更丰富的表现力。目前只有 `HandWaveSmooth` 和 `JumpBounce` 使用了高级缓动类型（如回弹、弹跳），其他动作都使用固定的 S 型查表曲线。

## Glossary

- **Otto**: 双足机器人平台，具有 4 个腿部舵机和可选的 2 个手部舵机
- **贝塞尔曲线 (Bezier Curve)**: 用于生成平滑运动轨迹的数学曲线
- **缓动类型 (EaseType)**: 定义运动加减速特性的枚举，包括 EASE_IN、EASE_OUT、EASE_IN_OUT、EASE_OUT_BACK、EASE_OUT_BOUNCE 等
- **振荡器 (Oscillator)**: 用于生成周期性运动的组件，基于正弦波
- **MoveServos**: 基础舵机移动函数，当前使用简单贝塞尔查表
- **MoveServosWithEase**: 支持指定缓动类型的舵机移动函数

## Requirements

### Requirement 1

**User Story:** As a robot user, I want the walking motion to feel more natural with smooth acceleration and deceleration, so that the robot moves more gracefully.

#### Acceptance Criteria

1. WHEN the Walk function is called THEN the Otto system SHALL use EASE_IN_OUT for leg movements during each step cycle
2. WHEN the Walk function completes a step THEN the Otto system SHALL use EASE_OUT for the final position adjustment
3. WHEN walking with hand swing enabled THEN the Otto system SHALL synchronize hand movements with leg movements using matching ease curves

### Requirement 2

**User Story:** As a robot user, I want the turning motion to have appropriate momentum effects, so that turns look more realistic.

#### Acceptance Criteria

1. WHEN the Turn function initiates a turn THEN the Otto system SHALL use EASE_IN for the starting phase
2. WHEN the Turn function completes a turn THEN the Otto system SHALL use EASE_OUT for the ending phase
3. WHEN turning with hand swing enabled THEN the Otto system SHALL apply coordinated ease curves to hand movements

### Requirement 3

**User Story:** As a robot user, I want the jump motion to have realistic physics-like behavior, so that jumps look more dynamic.

#### Acceptance Criteria

1. WHEN the Jump function performs the upward phase THEN the Otto system SHALL use EASE_OUT for explosive takeoff
2. WHEN the Jump function performs the downward phase THEN the Otto system SHALL use EASE_IN to simulate gravity acceleration
3. WHEN the Jump function lands THEN the Otto system SHALL use EASE_OUT_BOUNCE for impact absorption effect

### Requirement 4

**User Story:** As a robot user, I want the bend motion to have smooth weight-shifting effects, so that bends appear balanced and controlled.

#### Acceptance Criteria

1. WHEN the Bend function shifts weight to one side THEN the Otto system SHALL use EASE_IN_OUT for smooth transition
2. WHEN the Bend function returns to center THEN the Otto system SHALL use EASE_OUT for natural recovery

### Requirement 5

**User Story:** As a robot user, I want the shake leg motion to have playful bouncy effects, so that it looks more expressive.

#### Acceptance Criteria

1. WHEN the ShakeLeg function lifts the leg THEN the Otto system SHALL use EASE_OUT_BACK for overshoot effect
2. WHEN the ShakeLeg function shakes the leg THEN the Otto system SHALL use EASE_IN_OUT for rhythmic motion
3. WHEN the ShakeLeg function returns to standing THEN the Otto system SHALL use EASE_OUT for stable landing

### Requirement 6

**User Story:** As a robot user, I want the swing and sway motions to feel fluid and dance-like, so that the robot appears more lively.

#### Acceptance Criteria

1. WHEN the Swing function oscillates THEN the Otto system SHALL use EASE_IN_OUT for pendulum-like motion
2. WHEN the TiptoeSwing function oscillates THEN the Otto system SHALL use EASE_IN_OUT for graceful tiptoeing
3. WHEN the UpDown function bounces THEN the Otto system SHALL use EASE_OUT_BOUNCE for springy effect

### Requirement 7

**User Story:** As a robot user, I want the moonwalker and crusaito dance moves to have expressive timing, so that they look more entertaining.

#### Acceptance Criteria

1. WHEN the Moonwalker function performs sliding motion THEN the Otto system SHALL use EASE_IN_OUT for smooth gliding
2. WHEN the Crusaito function performs the dance THEN the Otto system SHALL use alternating EASE_IN and EASE_OUT for dynamic rhythm

### Requirement 8

**User Story:** As a robot user, I want the hand wave motion to have natural arm dynamics, so that waves look friendly and inviting.

#### Acceptance Criteria

1. WHEN the HandWave function raises the hand THEN the Otto system SHALL use EASE_OUT_BACK for enthusiastic lift
2. WHEN the HandWave function waves side to side THEN the Otto system SHALL use EASE_IN_OUT for smooth oscillation
3. WHEN the HandWave function lowers the hand THEN the Otto system SHALL use EASE_IN_OUT for gentle descent

### Requirement 9

**User Story:** As a robot user, I want the home position return to be smooth and gentle, so that the robot settles naturally.

#### Acceptance Criteria

1. WHEN the Home function returns servos to rest position THEN the Otto system SHALL use EASE_IN_OUT for smooth settling
2. WHEN the Home function completes THEN the Otto system SHALL ensure all servos reach exact target positions

### Requirement 10

**User Story:** As a developer, I want backward compatibility maintained, so that existing code continues to work without modification.

#### Acceptance Criteria

1. WHEN existing motion functions are called with current parameters THEN the Otto system SHALL produce visually similar motion patterns
2. WHEN the MoveServos function is called THEN the Otto system SHALL continue to use the default EASE_IN_OUT behavior
3. WHEN optional ease type parameters are not provided THEN the Otto system SHALL use sensible defaults matching current behavior
