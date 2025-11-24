# 🎨 Otto 机器人眼睛显示混合模式调整指南

## 当前配置

**文件**：`main/boards/otto-robot/otto_emoji_display.cc`  
**位置**：第 101-105 行

```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```

---

## 问题：整个屏幕都变黄了？

**原因**：GIF 重新着色会影响整个图像（包括暗部背景）

**解决方案**：
1. ✅ 背景设为纯黑色
2. ✅ 降低重新着色强度到 50%
3. ✅ 使用加法混合模式（`ADDITIVE`）

---

## 🎨 可用的混合模式

### 1. **`LV_BLEND_MODE_ADDITIVE`**（当前）
```cpp
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```

**效果**：
- 黑色（RGB 0,0,0）+ 黄色 = **黑色**（保持黑色）✅
- 白色（RGB 255,255,255）+ 黄色 = **亮黄色**（眼睛）✅

**适用**：背景纯黑，眼睛明亮的场景（**推荐**）

---

### 2. **`LV_BLEND_MODE_MULTIPLY`**（相乘模式）
```cpp
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_MULTIPLY, 0);
```

**效果**：
- 黑色 × 黄色 = **黑色**（保持黑色）✅
- 白色 × 黄色 = **黄色**（眼睛）✅
- 灰色 × 黄色 = **暗黄色**（中间调）

**适用**：想要更柔和的黄色效果

**需要同时调整**：
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_70, 0);  // 提高到 70%
```

---

### 3. **`LV_BLEND_MODE_NORMAL`**（默认模式）
```cpp
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_NORMAL, 0);
// 或者直接注释掉这一行
```

**效果**：
- 普通混合，暗部会有轻微黄色污染

**需要同时调整**：
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_30, 0);  // 降到 30%
```

**适用**：想要非常淡的黄色提示

---

### 4. **`LV_BLEND_MODE_SCREEN`**（滤色模式）
```cpp
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_SCREEN, 0);
```

**效果**：
- 类似加法模式，但更柔和
- 黑色保持黑色，亮部更柔和

**适用**：想要柔和的发光效果

---

## 📊 推荐配置组合

### 方案 A：鲜明黄色眼睛（推荐）✨
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```
**效果**：眼睛金黄色，背景纯黑

---

### 方案 B：柔和黄色眼睛
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_70, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_MULTIPLY, 0);
```
**效果**：眼睛温暖黄色，更自然

---

### 方案 C：淡黄色提示
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_30, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_NORMAL, 0);
```
**效果**：眼睛微微泛黄，接近原始白色

---

### 方案 D：不重新着色（纯白眼睛）
```cpp
// 注释掉所有重新着色代码
// lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
// lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
// lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```
**效果**：眼睛保持原始灰度（白色）

---

## 🔧 快速调整步骤

### 1. 打开文件
```
main/boards/otto-robot/otto_emoji_display.cc
```

### 2. 找到第 101-105 行
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);  // ← 调整强度
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);  // ← 调整颜色
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);  // ← 调整混合模式
```

### 3. 尝试不同组合
根据上面的方案表，修改这三行代码

### 4. 编译烧录
```bash
idf.py build flash
```

### 5. 观察效果
看看眼睛和背景的显示效果是否符合预期

---

## 🎨 颜色调整

如果想尝试其他颜色，修改**第 102 行**：

```cpp
// 黄色系
0xFFD700  // 金黄色（当前）
0xFFFF00  // 亮黄色
0xFFA500  // 橙黄色

// 其他颜色
0x00FF00  // 荧光绿
0x00E5FF  // 青色
0xFF0000  // 红色
0xFFFFFF  // 纯白色（恢复原始）
```

---

## 📸 效果对比

| 配置 | 眼睛效果 | 背景效果 | 适用场景 |
|------|---------|---------|---------|
| **ADDITIVE + 50%** | 金黄色，鲜明 | 纯黑色 | 需要明显区分（推荐）|
| **MULTIPLY + 70%** | 温暖黄色，柔和 | 纯黑色 | 自然、舒适的效果 |
| **NORMAL + 30%** | 淡黄色，接近白色 | 纯黑色 | 轻微提示颜色 |
| **不着色** | 原始白色/灰色 | 纯黑色 | 经典黑白风格 |

---

## ⚠️ 故障排查

### 问题 1：背景还是有黄色
**原因**：重新着色强度太高  
**解决**：
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_30, 0);  // 降到 30%
```

### 问题 2：眼睛颜色太淡
**原因**：重新着色强度太低  
**解决**：
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_70, 0);  // 提高到 70%
```

### 问题 3：眼睛看起来太亮/刺眼
**原因**：ADDITIVE 模式 + 高强度  
**解决**：
```cpp
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_MULTIPLY, 0);  // 改用 MULTIPLY
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_60, 0);
```

### 问题 4：想恢复原始白色眼睛
**解决**：注释掉重新着色代码
```cpp
// lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
// lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
// lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```

---

## 🌟 推荐最终配置

根据大多数用户反馈，以下配置效果最佳：

```cpp
// 🎨 方案 A（推荐）：鲜明金黄色眼睛 + 纯黑背景
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
lv_obj_set_style_blend_mode(emotion_gif_, LV_BLEND_MODE_ADDITIVE, 0);
```

**效果**：
- ✅ 眼睛：醒目的金黄色
- ✅ 背景：纯黑色，无污染
- ✅ 对比度：高对比，易于识别机器人状态

---

**创建时间**：2025-10-17  
**文件位置**：`main/boards/otto-robot/otto_emoji_display.cc` (第 101-105 行)  
**当前配置**：ADDITIVE 混合 + 50% 不透明度 + 金黄色 #FFD700


