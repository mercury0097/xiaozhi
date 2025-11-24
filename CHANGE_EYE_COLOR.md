# 👁️ Otto 机器人眼睛颜色切换指南

## 🎨 当前设置

**默认颜色**：金黄色 (#FFD700)  
**重新着色强度**：80%

---

## 🌈 快速换色

打开文件 `main/boards/otto-robot/otto_emoji_display.cc`，找到第 **98-99** 行：

```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_80, 0);  // 强度
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);  // 颜色
```

### 方法 1：修改颜色代码

替换 `0xFFD700` 为您想要的颜色：

```cpp
// ⭐ 黄色系
0xFFFF00  // 亮黄色（柠檬黄）
0xFFD700  // 金黄色（当前）
0xFFA500  // 橙黄色
0xFF8C00  // 深橙色

// 💚 绿色系
0x00FF00  // 荧光绿（类似 Matrix）
0x00FF7F  // 春绿色
0x32CD32  // 石灰绿
0x7FFF00  // 黄绿色

// 💙 蓝色系
0x00A0FF  // 天蓝色
0x1E90FF  // 道奇蓝
0x00BFFF  // 深天蓝
0x87CEEB  // 天空蓝

// ❤️ 红色系
0xFF0000  // 纯红色（警告效果）
0xFF1744  // 鲜红色
0xFF6347  // 番茄红
0xFF4500  // 橙红色

// 💜 紫色系
0x9400D3  // 深紫色
0xFF00FF  // 洋红色
0xBA55D3  // 中兰花紫
0xDA70D6  // 兰花紫

// 💎 青色系
0x00FFFF  // 青色（霓虹灯效果）
0x00E5FF  // 亮青色
0x40E0D0  // 绿松石色
0x48D1CC  // 中绿松石色

// ⚪ 中性色
0xFFFFFF  // 纯白色（原始效果）
0xC0C0C0  // 银色
0xFFFACD  // 柠檬绸色
0xFFE4B5  // 鹿皮色
```

---

## 🎛️ 调整强度

修改第 **98** 行的不透明度值：

```cpp
// 强度等级（0-255，或使用预定义常量）
LV_OPA_0     // 0%   - 无效果（显示原始灰度）
LV_OPA_10    // 10%  - 微弱着色
LV_OPA_30    // 30%  - 淡淡的颜色
LV_OPA_50    // 50%  - 半透明效果
LV_OPA_70    // 70%  - 较强着色
LV_OPA_80    // 80%  - 当前设置
LV_OPA_90    // 90%  - 强烈着色
LV_OPA_100   // 100% - 完全着色（最鲜艳）
```

### 推荐组合

```cpp
// 🌟 柔和金黄（适合日常）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_70, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);

// ⚡ 霓虹青色（炫酷效果）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_90, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00E5FF), 0);

// 💚 Matrix 绿（黑客风格）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_95, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00FF00), 0);

// ❤️ 警报红（错误状态）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_85, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFF1744), 0);

// 💙 科幻蓝（未来感）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_80, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00A0FF), 0);

// ⚪ 纯净白（极简风格）
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_50, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFFFFF), 0);
```

---

## 🎨 RGB 颜色计算器

如果您有特定的 RGB 值，转换为十六进制：

```
公式：0xRRGGBB

例如：
RGB(255, 215, 0) → 0xFFD700  （金黄色）
RGB(0, 255, 0)   → 0x00FF00  （荧光绿）
RGB(0, 160, 255) → 0x00A0FF  （天蓝色）
```

### 在线工具
- https://www.rapidtables.com/web/color/RGB_Color.html
- https://htmlcolorcodes.com/

---

## 📝 修改步骤

### 1. 编辑文件
```bash
# 在 Cursor 或任何编辑器中打开
main/boards/otto-robot/otto_emoji_display.cc
```

### 2. 找到第 98-99 行
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_80, 0);  // ← 这里
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);  // ← 和这里
```

### 3. 修改颜色代码
```cpp
// 示例：改为荧光绿
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00FF00), 0);
```

### 4. 编译烧录
```bash
# 在 IDE 中点击编译烧录按钮，或使用命令行
idf.py build flash
```

### 5. 查看效果
重启后，眼睛应该显示新的颜色！

---

## 🎭 进阶：动态换色

如果想根据不同表情使用不同颜色，可以修改 `SetEmotion()` 函数：

```cpp
void OttoEmojiDisplay::SetEmotion(const char* emotion) {
    if (!emotion || !emotion_gif_) {
        return;
    }

    DisplayLockGuard lock(this);

    // 根据表情设置不同颜色
    uint32_t eye_color = 0xFFD700;  // 默认金黄色
    
    if (strcmp(emotion, "happy") == 0) {
        eye_color = 0x00FF00;  // 开心 = 荧光绿
    } else if (strcmp(emotion, "sad") == 0) {
        eye_color = 0x1E90FF;  // 悲伤 = 蓝色
    } else if (strcmp(emotion, "angry") == 0) {
        eye_color = 0xFF0000;  // 愤怒 = 红色
    } else if (strcmp(emotion, "surprised") == 0) {
        eye_color = 0xFF00FF;  // 惊讶 = 洋红
    }
    
    // 应用颜色
    lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(eye_color), 0);

    // 设置 GIF
    for (const auto& map : emotion_maps_) {
        if (map.name && strcmp(map.name, emotion) == 0) {
            lv_gif_set_src(emotion_gif_, map.gif);
            ESP_LOGI(TAG, "设置表情: %s（颜色=0x%06X）", emotion, eye_color);
            return;
        }
    }

    lv_gif_set_src(emotion_gif_, &staticstate);
    ESP_LOGI(TAG, "未知表情'%s'，使用默认", emotion);
}
```

---

## 🎨 推荐配色方案

### 1. **经典黄金** ⭐（默认）
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_80, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFFD700), 0);
```
**效果**：温暖、友好、经典机器人风格

---

### 2. **霓虹青** 💎
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_90, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00E5FF), 0);
```
**效果**：科幻、炫酷、未来感

---

### 3. **Matrix 绿** 💚
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_95, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x00FF00), 0);
```
**效果**：黑客风格、神秘、科技感

---

### 4. **天空蓝** 💙
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_80, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0x1E90FF), 0);
```
**效果**：清新、冷静、现代

---

### 5. **警报红** ❤️
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_85, 0);
lv_obj_set_style_img_recolor(emotion_gif_, lv_color_hex(0xFF1744), 0);
```
**效果**：警示、紧急、高能量

---

## 🛠️ 故障排查

### 问题 1：修改后颜色没变
**原因**：未重新编译  
**解决**：
```bash
idf.py build flash
```

### 问题 2：颜色太淡
**原因**：不透明度太低  
**解决**：提高到 `LV_OPA_90` 或更高

### 问题 3：颜色过于鲜艳刺眼
**原因**：不透明度太高  
**解决**：降低到 `LV_OPA_60` 或 `LV_OPA_70`

### 问题 4：想恢复原始灰度
**解决**：
```cpp
lv_obj_set_style_img_recolor_opa(emotion_gif_, LV_OPA_0, 0);
```

---

## 📸 效果预览

| 颜色代码 | 颜色名称 | 视觉效果 | 适用场景 |
|---------|---------|---------|---------|
| `0xFFD700` | 金黄色 | 温暖友好 | 日常使用 |
| `0x00FF00` | 荧光绿 | 酷炫神秘 | 科技展示 |
| `0x00E5FF` | 霓虹青 | 未来感强 | 科幻主题 |
| `0xFF0000` | 纯红色 | 警示醒目 | 错误提示 |
| `0x1E90FF` | 道奇蓝 | 清新冷静 | 办公环境 |
| `0xFF00FF` | 洋红色 | 梦幻活泼 | 儿童模式 |
| `0xFFFFFF` | 纯白色 | 极简纯净 | 极简风格 |

---

**创建时间**：2025-10-17  
**文件位置**：`main/boards/otto-robot/otto_emoji_display.cc` (第 98-99 行)  
**当前颜色**：金黄色 (#FFD700)

