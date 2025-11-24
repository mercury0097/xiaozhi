# 🚀 VSCode 编译烧录指南（已自动保护 VADNet1 补丁）

## ✅ 现在可以安全使用 VSCode 了！

从现在开始，您可以直接在 VSCode 中点击 ESP-IDF 扩展的按钮进行编译和烧录，**不需要任何额外操作**。

---

## 🛡️ 自动保护机制说明

### 工作原理
我已经在 `CMakeLists.txt` 中添加了自动检查和恢复机制：

```cmake
# 🛡️ 自动检查并应用 VADNet1 补丁（防止 managed_components 修改丢失）
# 每次运行 idf.py build 时，自动检查补丁是否存在
# 如果补丁丢失，会自动重新应用
```

### 触发时机
这个检查会在以下情况下自动执行：
- ✅ 在 VSCode 中点击"构建"按钮（🔨）
- ✅ 在终端运行 `idf.py build`
- ✅ 在 VSCode 中点击"构建、烧录、监视器"（⚡）
- ✅ 每次 CMake 重新配置时

### 保护的内容
- ✅ VADNet1 神经网络 VAD 补丁
- ✅ 防止 `model_path.c` 的静态缓存问题
- ✅ 确保每次重启后都能正确加载 `vadnet1_medium` 模型

---

## 📋 VSCode 推荐工作流程

### 方式 1：使用 ESP-IDF 扩展按钮（推荐）

1. **编译**：点击底部状态栏的 **🔨 Build** 按钮
   - 自动检查并应用 VADNet1 补丁
   - 增量编译（只编译修改过的文件）
   - 速度快，适合日常开发

2. **烧录**：点击 **⚡ Flash** 按钮
   - 自动烧录所有必要的分区（app + model + assets）
   - 无需手动指定地址

3. **监视器**：点击 **📟 Monitor** 按钮
   - 查看设备日志
   - 按 `Ctrl+]` 退出

4. **一键完成**：点击 **⚡ Build, Flash and Monitor**
   - 一次性完成编译、烧录、监视器

### 方式 2：使用终端命令

```bash
# 进入项目目录
cd /Users/machenyang/Desktop/xiaozhi-esp32-main

# 编译
idf.py build

# 烧录
idf.py flash

# 监视器
idf.py monitor

# 或者一键完成
idf.py build flash monitor
```

---

## ⚠️ 何时需要手动干预

在以下情况下，自动保护可能失效，需要手动恢复：

### 情况 1：删除了 `managed_components` 目录
```bash
# 如果手动删除了整个 managed_components 目录
# 重新构建后，自动应用补丁：
idf.py build
```

### 情况 2：更新了 ESP-SR 组件版本
```bash
# 如果修改了 main/idf_component.yml 中的 esp-sr 版本
# 运行以下命令更新依赖后，补丁会自动重新应用：
idf.py update-dependencies
idf.py build
```

### 情况 3：手动运行了 `idf.py fullclean`
```bash
# fullclean 会删除所有构建文件和依赖
# 重新构建时，补丁会自动重新应用：
idf.py fullclean
idf.py build
```

### 情况 4：补丁脚本或补丁文件被删除
如果意外删除了以下文件，请手动恢复：
- `patches/fix_vadnet1_cache.patch`
- `apply_vadnet1_fix.sh`

恢复方法：查看 `VADNet1_修复说明.md` 中的手动修复步骤。

---

## 🔍 如何验证补丁是否生效

### 方法 1：查看构建日志

在 VSCode 的"终端"或"输出"面板中，查找以下内容：

```
-- ✅ VADNet1 补丁已存在，无需重新应用
```

或者（如果补丁丢失并自动恢复）：
```
-- ⚠️  检测到 VADNet1 补丁缺失，正在自动应用...
-- ✅ VADNet1 补丁已自动应用成功
```

### 方法 2：查看设备日志

烧录后，在监视器中说"你好小智"触发唤醒，查找以下关键日志：

```
[PATCH] Clear old static_srmodels cache before loading from Flash  ← 补丁生效标记
ESP-SR 模型 2: vadnet1_medium  ← 模型已加载
Set VAD Model: vadnet1_medium  ← VAD 模型已设置
AFE Pipeline: ... |VAD(vadnet1_medium)| ...  ← VAD 已启用
MC Quantized vadnet1:vadnet1_mediumv1_Speech...  ← VADNet1 初始化成功
```

---

## 🚫 不需要做的事情

### ❌ 不需要每次都删除 build 目录
ESP-IDF 的增量编译非常可靠，只需要在以下情况下清理：
- 修改了 `sdkconfig`（配置文件）
- 遇到奇怪的链接错误
- 想要完全重新构建

### ❌ 不需要手动运行补丁脚本
`apply_vadnet1_fix.sh` 现在会被 CMake 自动调用，除非您想手动测试。

### ❌ 不需要手动烧录 `srmodels.bin`
VSCode 的"Flash"按钮会自动烧录所有必要的分区，包括：
- `bootloader.bin` （引导程序）
- `partition-table.bin` （分区表）
- `xiaozhi.bin` （主程序）
- `srmodels.bin` （ESP-SR 模型）
- `generated_assets.bin` （资源文件）

---

## 🔓 解决串口占用问题

如果烧录时提示串口被占用（`port is busy`），使用以下方法：

### 方法 1：使用一键释放脚本（推荐）⭐
```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
./释放串口.sh
```

### 方法 2：手动查找并关闭
```bash
# 查找占用串口的进程
lsof | grep tty.usbmodem101

# 关闭进程（将 <PID> 替换为实际的进程号）
kill -9 <PID>
```

### 方法 3：一行命令解决（复制即用）
```bash
kill -9 $(lsof 2>/dev/null | grep tty.usbmodem101 | awk '{print $2}' | sort -u)
```

---

## 📝 常见问题

### Q1: 点击"Build"后没有看到补丁相关的日志？
**A**: 这是正常的！如果补丁已存在，日志会显示在 CMake 的配置阶段，可能被其他日志覆盖。只要编译成功，且设备日志中看到 `[PATCH]` 标记，就说明补丁生效。

### Q2: 为什么有时候编译很快，有时候很慢？
**A**: 
- **快**：增量编译，只编译修改过的文件（通常 5-30 秒）
- **慢**：完全重新编译，需要编译所有文件（通常 2-5 分钟）

触发完全重新编译的情况：
- 修改了 `sdkconfig`
- 修改了 `CMakeLists.txt`
- 运行了 `idf.py fullclean`
- 删除了 `build` 目录

### Q3: 可以在 VSCode 中删除 build 目录吗？
**A**: 可以！删除后再次编译，补丁会自动重新应用。但通常不需要删除，除非：
- 遇到无法解决的编译错误
- 想要确保一个干净的构建

### Q4: 修改了 `afe_ringbuf_size` 后需要删除 build 吗？
**A**: 不需要！只需：
1. 修改 `main/audio/processors/afe_audio_processor.cc`
2. 点击"Build"按钮（增量编译）
3. 点击"Flash"按钮烧录

---

## ✅ 总结

**您现在可以像使用任何其他 ESP-IDF 项目一样使用 VSCode**：

1. ✅ 修改代码
2. ✅ 点击"Build"按钮（🔨）
3. ✅ 点击"Flash"按钮（⚡）
4. ✅ 点击"Monitor"按钮（📟）

**VADNet1 补丁会自动保护，无需担心丢失！** 🎉

---

## 📚 相关文档

- `VADNet1_修复说明.md` - 补丁的详细技术说明
- `patches/fix_vadnet1_cache.patch` - 补丁文件本身
- `apply_vadnet1_fix.sh` - 手动应用补丁的脚本
- `README.md` - 项目主文档

---

**最后更新**: 2025-10-22  
**状态**: ✅ 已完成并经过测试

