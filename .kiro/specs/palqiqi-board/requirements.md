# Requirements Document

## Introduction

本需求文档描述了将现有的 otto-robot 板子类型复制并重命名为 palqiqi 的功能需求。由于在 otto-robot 基础上已经进行了大量定制修改，需要创建一个独立的板子类型以避免未来 OTA 升级时被原有固件覆盖，同时保持项目的独立性和可维护性。

## Glossary

- **Board Type**: 开发板类型，ESP32 项目中用于区分不同硬件配置的标识
- **palqiqi**: 新的自定义开发板名称，用于替代 otto-robot
- **Kconfig**: ESP-IDF 的配置系统，用于定义编译选项
- **CMakeLists.txt**: CMake 构建系统配置文件
- **config.json**: 板级编译配置文件，用于 release.py 脚本
- **config.h**: 板级硬件配置头文件，定义 GPIO 引脚映射
- **DECLARE_BOARD**: 宏定义，用于向系统注册开发板类型

## Requirements

### Requirement 1

**User Story:** As a developer, I want to create a new board directory named palqiqi, so that I can have an independent board type separate from otto-robot.

#### Acceptance Criteria

1. WHEN the developer creates the palqiqi board THEN the System SHALL create a new directory at `main/boards/palqiqi` containing all necessary files
2. WHEN the palqiqi directory is created THEN the System SHALL copy all files from `main/boards/otto-robot` including subdirectories
3. WHEN files are copied THEN the System SHALL preserve the complete directory structure including the `vector_eyes` subdirectory

### Requirement 2

**User Story:** As a developer, I want all otto-related naming to be replaced with palqiqi, so that the new board has a consistent and unique identity.

#### Acceptance Criteria

1. WHEN renaming files THEN the System SHALL rename `otto_robot.cc` to `palqiqi_board.cc`
2. WHEN renaming files THEN the System SHALL rename `otto_controller.cc` to `palqiqi_controller.cc`
3. WHEN renaming files THEN the System SHALL rename `otto_movements.h` to `palqiqi_movements.h`
4. WHEN renaming files THEN the System SHALL rename `otto_movements.cc` to `palqiqi_movements.cc`
5. WHEN renaming files THEN the System SHALL rename `otto_emoji_display.h` to `palqiqi_emoji_display.h`
6. WHEN renaming files THEN the System SHALL rename `otto_emoji_display.cc` to `palqiqi_emoji_display.cc`
7. WHEN renaming files THEN the System SHALL rename `otto_vector_eye_display.h` to `palqiqi_vector_eye_display.h`
8. WHEN renaming files THEN the System SHALL rename `otto_vector_eye_display.cc` to `palqiqi_vector_eye_display.cc`

### Requirement 3

**User Story:** As a developer, I want all class names and identifiers to be updated to palqiqi, so that the code is internally consistent.

#### Acceptance Criteria

1. WHEN updating class names THEN the System SHALL rename class `OttoRobot` to `PalqiqiBoard`
2. WHEN updating class names THEN the System SHALL rename class `Otto` to `Palqiqi` in movements files
3. WHEN updating class names THEN the System SHALL rename class `OttoController` to `PalqiqiController`
4. WHEN updating class names THEN the System SHALL rename class `OttoEmojiDisplay` to `PalqiqiEmojiDisplay`
5. WHEN updating class names THEN the System SHALL rename class `OttoVectorEyeDisplay` to `PalqiqiVectorEyeDisplay`
6. WHEN updating identifiers THEN the System SHALL update all LOG TAGs from "Otto" related strings to "Palqiqi" related strings
7. WHEN updating identifiers THEN the System SHALL update all function names containing "Otto" to use "Palqiqi"

### Requirement 4

**User Story:** As a developer, I want the config.json to be updated for palqiqi, so that the build system recognizes the new board.

#### Acceptance Criteria

1. WHEN updating config.json THEN the System SHALL change the build name from "otto-robot" to "palqiqi"
2. WHEN updating config.json THEN the System SHALL preserve all other build configurations including target and sdkconfig_append

### Requirement 5

**User Story:** As a developer, I want the Kconfig.projbuild to include palqiqi as a board option, so that I can select it during configuration.

#### Acceptance Criteria

1. WHEN adding Kconfig option THEN the System SHALL add a new config entry `BOARD_TYPE_PALQIQI`
2. WHEN adding Kconfig option THEN the System SHALL set the board description to "Palqiqi Robot"
3. WHEN adding Kconfig option THEN the System SHALL set the dependency to `IDF_TARGET_ESP32S3`
4. WHEN adding Kconfig option THEN the System SHALL select `LV_USE_GIF` and `LV_GIF_CACHE_DECODE_DATA`

### Requirement 6

**User Story:** As a developer, I want the CMakeLists.txt to include palqiqi board configuration, so that the build system can compile the new board.

#### Acceptance Criteria

1. WHEN adding CMakeLists configuration THEN the System SHALL add an elseif block for `CONFIG_BOARD_TYPE_PALQIQI`
2. WHEN adding CMakeLists configuration THEN the System SHALL set `BOARD_TYPE` to "palqiqi"
3. WHEN adding CMakeLists configuration THEN the System SHALL set the same font configurations as otto-robot (`font_puhui_16_4` and `font_awesome_16_4`)

### Requirement 7

**User Story:** As a developer, I want the project to compile and run correctly after the changes, so that the palqiqi board functions identically to the original otto-robot.

#### Acceptance Criteria

1. WHEN the configuration is complete THEN the System SHALL allow successful compilation when palqiqi board is selected
2. WHEN the board is registered THEN the System SHALL use `DECLARE_BOARD(PalqiqiBoard)` macro correctly
3. WHEN include paths are updated THEN the System SHALL correctly reference all renamed header files
