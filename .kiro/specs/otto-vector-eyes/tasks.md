# Otto 矢量眼睛系统实现任务

## 阶段 1: 基础框架搭建

### Task 1.1: 创建目录结构和基础文件
- [ ] 创建 `main/boards/otto-robot/vector_eyes/` 目录
- [ ] 创建 `eye_config.h` - 眼睛参数结构定义
- [ ] 创建 `eye_presets.h` - 18 种表情预设参数
- [ ] 创建 `emotions.h` - 表情枚举定义
- [ ] 更新 CMakeLists.txt 添加新源文件

### Task 1.2: 实现时间工具函数
- [ ] 创建 `vector_eyes/utils.h`
- [ ] 实现 `millis_idf()` 替代 Arduino 的 `millis()`
- [ ] 实现基础缓动函数 (EaseInOutQuad)

## 阶段 2: 核心绘制引擎

### Task 2.1: 实现 LVGL 眼睛绘制器
- [ ] 创建 `eye_drawer.h/.cc`
- [ ] 实现 `FillRectangle()` - 矩形填充
- [ ] 实现 `FillTriangle()` - 三角形填充
- [ ] 实现 `FillEllipseCorner()` - 圆角绘制
- [ ] 实现 `Draw()` - 完整眼睛绘制逻辑
- [ ] 使用 lv_canvas 作为绘图目标

### Task 2.2: 实现单眼类 (VectorEye)
- [ ] 创建 `vector_eye.h/.cc`
- [ ] 实现眼睛参数存储和管理
- [ ] 实现 `ApplyPreset()` - 立即应用预设
- [ ] 实现 `Draw()` - 调用绘制器绘制
- [ ] 支持镜像模式 (左眼)

## 阶段 3: 动画系统

### Task 3.1: 实现表情过渡动画
- [ ] 创建 `eye_transition.h/.cc`
- [ ] 实现参数插值 `Lerp()`
- [ ] 实现 `TransitionTo()` - 启动过渡
- [ ] 实现 `Update()` - 更新过渡状态
- [ ] 支持可配置过渡时长

### Task 3.2: 实现眨眼控制器
- [ ] 创建 `blink_controller.h/.cc`
- [ ] 实现随机眨眼间隔计算
- [ ] 实现眨眼动画 (快闭慢睁)
- [ ] 实现 `Blink()` - 手动触发眨眼
- [ ] 实现 `Update()` - 更新眨眼状态

### Task 3.3: 实现视线控制器
- [ ] 创建 `look_controller.h/.cc`
- [ ] 实现 `LookAt(x, y)` - 看向指定方向
- [ ] 实现随机视线移动
- [ ] 实现视线平滑过渡

## 阶段 4: 整合与显示

### Task 4.1: 实现 VectorFace 双眼管理
- [ ] 创建 `vector_face.h/.cc`
- [ ] 管理左右眼实例
- [ ] 整合眨眼和视线控制器
- [ ] 实现 `SetExpression()` - 设置表情
- [ ] 实现 `Update()` - 统一更新

### Task 4.2: 实现 OttoVectorEyeDisplay
- [ ] 创建 `otto_vector_eye_display.h/.cc`
- [ ] 继承 SpiLcdDisplay
- [ ] 创建 lv_canvas 用于绘制
- [ ] 实现 `SetEmotion()` - 表情名映射
- [ ] 实现定时更新任务 (30Hz)
- [ ] 保留聊天消息显示功能

### Task 4.3: 表情名称映射
- [ ] 创建完整的表情名 -> 预设映射表
- [ ] 映射现有 21 种表情名到 18 种预设
- [ ] 处理未知表情名 (fallback 到 Normal)

## 阶段 5: 集成与切换

### Task 5.1: 修改 otto_board.cc 支持切换
- [ ] 添加 `USE_VECTOR_EYES` 编译开关
- [ ] 条件编译选择显示类
- [ ] 确保两种模式都能编译通过

### Task 5.2: 更新 CMakeLists.txt
- [ ] 添加所有新源文件
- [ ] 配置条件编译选项
- [ ] 确保依赖正确

## 阶段 6: 测试与优化

### Task 6.1: 功能测试
- [ ] 测试所有 18 种表情显示
- [ ] 测试表情过渡动画
- [ ] 测试眨眼功能
- [ ] 测试视线移动
- [ ] 测试 GIF 模式回退

### Task 6.2: 性能优化
- [ ] 测量帧率，确保 >= 30 FPS
- [ ] 优化绘制算法减少 CPU 占用
- [ ] 检查内存使用

### Task 6.3: Git 管理
- [ ] 提交前确保 GIF 代码完整保留
- [ ] 创建有意义的 commit 信息
- [ ] 可选：创建 feature 分支

---

## 实现顺序建议

1. Task 1.1 → 1.2 (基础框架)
2. Task 2.1 → 2.2 (能画出静态眼睛)
3. Task 4.2 (集成到显示系统，可以看到效果)
4. Task 3.1 → 4.1 (添加过渡动画)
5. Task 3.2 → 3.3 (添加眨眼和视线)
6. Task 4.3 → 5.1 → 5.2 (完整集成)
7. Task 6.x (测试优化)

## 风险点

1. LVGL canvas 绘制性能可能不如 U8G2 直接操作帧缓冲
2. 圆角绘制算法可能需要调整以适应 LVGL
3. 240x240 屏幕尺寸与原库 128x64 差异较大，参数需要缩放
