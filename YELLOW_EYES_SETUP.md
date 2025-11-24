# 🟡 黄色眼睛 GIF 表情 - 安装完成

## ✅ 已完成的修改

### 1. GIF 颜色转换
已成功将所有 6 个表情的 GIF 颜色修改为：
- **白色/亮灰色** → **亮黄色** (#FFD700 金色)
- **中等灰色** → **渐变黄色**
- **深灰色** → **纯黑色** (#000000)

### 2. 本地组件固定
为了防止组件管理器恢复原始文件，已将修改后的 GIF 组件固定到本地：

```
components/otto-emoji-gif-local/
├── CMakeLists.txt
├── include/
│   └── otto_emoji_gif.h
└── src/
    ├── staticstate.c  ✅ 黄色眼睛
    ├── happy.c        ✅ 黄色眼睛
    ├── sad.c          ✅ 黄色眼睛
    ├── anger.c        ✅ 黄色眼睛
    ├── scare.c        ✅ 黄色眼睛
    └── buxue.c        ✅ 黄色眼睛
```

### 3. 组件依赖配置
已修改 `main/idf_component.yml`，禁用远程组件，使用本地版本。

---

## 🚀 编译和烧录步骤

### 方法 1：使用您常用的编译工具
如果您使用 IDE（如 VS Code + ESP-IDF 插件）：
1. **清理项目**：删除 `build` 目录或使用 "ESP-IDF: Full Clean"
2. **重新构建**：点击 "Build"
3. **烧录**：点击 "Flash"

### 方法 2：使用命令行
在终端中执行（确保已激活 ESP-IDF 环境）：

```bash
# 进入项目目录
cd /Users/machenyang/Desktop/xiaozhi-esp32-main

# 清理构建
rm -rf build

# 重新编译
idf.py build

# 烧录到设备
idf.py flash
```

---

## 🎨 预期效果

烧录后，机器人的所有表情都会显示：
- ✅ **背景**：纯黑色 ⬛
- ✅ **眼睛**：亮金黄色 🟡 (#FFD700)
- ✅ **过渡区域**：自然的黄色渐变

### 表情切换场景
| 表情 | 触发场景 | 眼睛颜色 |
|------|---------|---------|
| 中性 (neutral) | 待机、监听 | 🟡 黄色 |
| 开心 (happy) | 回复成功 | 🟡 黄色 |
| 悲伤 (sad) | 错误、低电量 | 🟡 黄色 |
| 愤怒 (anger) | 特殊场景 | 🟡 黄色 |
| 惊吓 (scare) | 特殊场景 | 🟡 黄色 |
| 困惑 (buxue) | 特殊场景 | 🟡 黄色 |

---

## 🔧 故障排除

### 如果烧录后眼睛仍然是白色：

#### 1. 确认本地组件被使用
编译时查看输出，应该看到：
```
-- Otto Emoji GIF Component:
--   Found 6 GIF source files
--   Include directory: .../components/otto-emoji-gif-local/include
```

#### 2. 完全清理构建
```bash
rm -rf build
rm -rf managed_components/txp666__otto-emoji-gif-component
```

#### 3. 检查组件依赖
确认 `main/idf_component.yml` 中的远程组件已被注释：
```yaml
# txp666/otto-emoji-gif-component:
#   version: 1.0.2
# 使用本地修改版（黄色眼睛）：components/otto-emoji-gif-local
```

---

## 📦 备份文件

原始 GIF 文件已自动备份为 `.c.backup` 后缀：
```
managed_components/txp666__otto-emoji-gif-component/src/
├── staticstate.c.backup
├── happy.c.backup
├── sad.c.backup
├── anger.c.backup
├── scare.c.backup
└── buxue.c.backup
```

如需恢复原始黑白效果：
1. 从 `main/idf_component.yml` 中取消注释远程组件
2. 删除 `components/otto-emoji-gif-local/` 目录
3. 重新编译

---

## 🎨 自定义其他颜色

如果想要其他颜色的眼睛，可以重新运行转换脚本并修改颜色代码：

```bash
# 编辑脚本中的颜色值
vim scripts/change_gif_eye_color.py

# 找到这行并修改颜色：
# data[idx] = 0xFF      # R (红色分量)
# data[idx+1] = 0xD7    # G (绿色分量)
# data[idx+2] = 0x00    # B (蓝色分量)

# 重新运行脚本
python3 scripts/change_gif_eye_color.py
```

**常用颜色代码**：
- 🔴 红色：`RGB(255, 0, 0)`
- 🟢 绿色：`RGB(0, 255, 0)`
- 🔵 蓝色：`RGB(0, 0, 255)`
- 🟡 黄色：`RGB(255, 215, 0)` ← 当前
- 🟣 紫色：`RGB(128, 0, 128)`
- 🟠 橙色：`RGB(255, 165, 0)`
- ⚪ 白色：`RGB(255, 255, 255)` ← 原始

---

## 📝 技术说明

### 为什么需要本地组件？
ESP-IDF 的组件管理器 (`idf_component.yml`) 会在每次构建时检查并下载远程组件。如果直接修改 `managed_components/` 下的文件，下次构建时可能会被覆盖。

通过将修改后的组件复制到 `components/` 目录并禁用远程依赖，可以确保修改永久生效。

### GIF 颜色修改原理
GIF 文件使用**调色板**（颜色表），包含最多 256 种颜色。修改调色板中的颜色值，所有使用该颜色的像素都会改变。

脚本通过以下步骤修改颜色：
1. 读取 GIF 的全局颜色表（从字节 13 开始）
2. 识别白色/灰色/黑色
3. 替换为对应的黄色/黑色
4. 重新生成 `.c` 文件

---

**完成配置！现在请重新编译并烧录，享受金黄色的眼睛吧！** 🟡✨


