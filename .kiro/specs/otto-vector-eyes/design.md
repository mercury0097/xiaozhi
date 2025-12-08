# Otto 矢量眼睛系统设计文档

## 架构概述

```
┌─────────────────────────────────────────────────────────────┐
│                    OttoVectorEyeDisplay                     │
│                  (继承 SpiLcdDisplay)                        │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  VectorFace │  │ BlinkCtrl   │  │   LookCtrl          │  │
│  │  (双眼管理) │  │ (眨眼控制)  │  │   (视线控制)        │  │
│  └──────┬──────┘  └─────────────┘  └─────────────────────┘  │
│         │                                                    │
│  ┌──────┴──────┐                                            │
│  │  VectorEye  │ x2 (左眼/右眼)                             │
│  │  - config   │                                            │
│  │  - transition│                                           │
│  │  - draw()   │                                            │
│  └─────────────┘                                            │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              LvglEyeDrawer (静态绘制工具)                ││
│  │  - 使用 lv_canvas 绘制矢量眼睛                          ││
│  │  - 圆角矩形、三角形、椭圆角                             ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## 文件结构

```
main/boards/otto-robot/
├── otto_emoji_display.h/.cc      # 原 GIF 版本 (保留)
├── otto_emoji_gif.h              # GIF 资源 (保留)
├── otto_vector_eye_display.h/.cc # 新矢量版本
├── vector_eyes/
│   ├── eye_config.h              # 眼睛参数结构
│   ├── eye_presets.h             # 18种表情预设
│   ├── eye_drawer.h/.cc          # LVGL 绘制实现
│   ├── vector_eye.h/.cc          # 单眼类
│   ├── vector_face.h/.cc         # 双眼管理
│   ├── eye_transition.h/.cc      # 过渡动画
│   ├── blink_controller.h/.cc    # 眨眼控制
│   └── look_controller.h/.cc     # 视线控制
└── otto_board.cc                 # 修改：支持切换显示模式
```

## 核心数据结构

### EyeConfig (眼睛参数)
```cpp
struct EyeConfig {
    int16_t offset_x;           // X偏移
    int16_t offset_y;           // Y偏移
    int16_t height;             // 高度
    int16_t width;              // 宽度
    float slope_top;            // 上眼睑斜度 (-1.0 ~ 1.0)
    float slope_bottom;         // 下眼睑斜度
    int16_t radius_top;         // 上圆角
    int16_t radius_bottom;      // 下圆角
    int16_t inverse_radius_top; // 内凹上圆角 (笑眯眯效果)
    int16_t inverse_radius_bottom;
};
```

### 表情映射
```cpp
// 现有表情名 -> 矢量预设
const EmotionMapping emotion_map[] = {
    {"neutral", Preset_Normal},
    {"happy", Preset_Happy},
    {"sad", Preset_Sad},
    {"angry", Preset_Angry},
    {"surprised", Preset_Surprised},
    // ... 完整映射
};
```

## 关键实现

### 1. LVGL 绘制适配 (LvglEyeDrawer)

原库使用 U8G2 的绘图 API，需要替换为 LVGL：

| U8G2 API | LVGL 替代方案 |
|----------|--------------|
| `u8g2.drawBox()` | `lv_canvas_draw_rect()` |
| `u8g2.drawTriangle()` | `lv_canvas_draw_polygon()` |
| `u8g2.drawHLine()` | `lv_canvas_draw_line()` |
| `u8g2.clearBuffer()` | `lv_canvas_fill_bg()` |

使用 `lv_canvas` 作为绘图目标，每帧重绘。

### 2. 时间函数适配

```cpp
// Arduino: millis()
// ESP-IDF: esp_timer_get_time() / 1000
inline uint32_t millis_idf() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}
```

### 3. 表情过渡动画

```cpp
void VectorEye::TransitionTo(const EyeConfig& target, uint32_t duration_ms) {
    start_config_ = current_config_;
    target_config_ = target;
    transition_start_ = millis_idf();
    transition_duration_ = duration_ms;
    transitioning_ = true;
}

void VectorEye::Update() {
    if (transitioning_) {
        float t = (millis_idf() - transition_start_) / (float)transition_duration_;
        t = std::min(1.0f, t);
        // 缓动函数
        t = EaseInOutQuad(t);
        // 插值所有参数
        current_config_ = Lerp(start_config_, target_config_, t);
        if (t >= 1.0f) transitioning_ = false;
    }
}
```

### 4. 眨眼实现

```cpp
void BlinkController::Update() {
    uint32_t now = millis_idf();
    if (!blinking_ && now - last_blink_ > next_blink_interval_) {
        StartBlink();
    }
    if (blinking_) {
        // 眨眼动画：快速闭眼 + 稍慢睁眼
        float t = (now - blink_start_) / (float)BLINK_DURATION;
        if (t < 0.4f) {
            // 闭眼阶段
            blink_factor_ = t / 0.4f;
        } else if (t < 1.0f) {
            // 睁眼阶段
            blink_factor_ = 1.0f - (t - 0.4f) / 0.6f;
        } else {
            blinking_ = false;
            blink_factor_ = 0.0f;
            ScheduleNextBlink();
        }
    }
}
```

### 5. 显示模式切换

在 `otto_board.cc` 中支持配置：

```cpp
#ifdef USE_VECTOR_EYES
    display_ = new OttoVectorEyeDisplay(...);
#else
    display_ = new OttoEmojiDisplay(...);  // GIF版本
#endif
```

## 渲染流程

```
1. SetEmotion("happy") 调用
   ↓
2. 查找表情映射 → Preset_Happy
   ↓
3. VectorFace::SetExpression(Preset_Happy)
   ↓
4. 左右眼启动过渡动画 TransitionTo()
   ↓
5. 定时器触发 Update() (30Hz)
   ↓
6. 更新过渡插值、眨眼、视线
   ↓
7. LvglEyeDrawer::Draw() 绘制到 canvas
   ↓
8. LVGL 刷新显示
```

## 性能优化

1. **脏区域更新**: 只重绘眼睛区域，不刷新整屏
2. **预计算**: 表情预设参数编译时确定
3. **定时器控制**: 30Hz 更新频率，避免过度渲染
4. **整数运算**: 绘制时尽量使用整数避免浮点

## 测试计划

1. 单元测试：EyeConfig 插值正确性
2. 集成测试：所有 18 种表情显示
3. 性能测试：帧率和 CPU 占用
4. 回归测试：GIF 模式仍可正常工作
