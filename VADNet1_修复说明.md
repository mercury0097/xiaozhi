# VADNet1 缓存修复说明

## 🎉 已自动化！（2025-10-22 更新）

**好消息**：现在补丁会在每次构建时**自动检查和应用**！

- ✅ 在 VSCode 中点击"构建"按钮时自动检查
- ✅ 运行 `idf.py build` 时自动检查
- ✅ 补丁丢失时自动恢复
- ✅ 无需手动干预

**详细说明**：请查看 `VSCode编译烧录指南.md`

---

## 🎯 修复内容

修复了 ESP-SR 的 `static_srmodels` 缓存问题，确保 VADNet1 神经网络 VAD 能够成功加载。

## 📁 相关文件

- **修改的文件**: `managed_components/espressif__esp-sr/src/model_path.c`
- **补丁备份**: `patches/fix_vadnet1_cache.patch`
- **自动修复脚本**: `apply_vadnet1_fix.sh`

## ⚠️ 何时需要重新应用补丁

修改会在以下情况下**被撤销**，需要重新应用：

1. 运行 `idf.py reconfigure` 或 `idf.py update-dependencies`
2. 删除 `managed_components` 目录
3. 组件版本更新

**不会被撤销**的情况：
- ✅ 正常编译 (`idf.py build`)
- ✅ 烧录 (`idf.py flash`)
- ✅ 监控 (`idf.py monitor`)

## 🔧 重新应用补丁的方法

### 方法 1：使用自动修复脚本（推荐）

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
./apply_vadnet1_fix.sh
```

### 方法 2：手动编辑

编辑 `managed_components/espressif__esp-sr/src/model_path.c`，在 `srmodel_mmap_init` 函数开头（`{` 之后）添加：

```c
// 🎯 强制清除旧缓存，确保每次都从 Flash 重新加载最新模型
// 修复：设备重启后 vadnet1_medium 无法加载的问题
if (static_srmodels != NULL) {
    printf("[PATCH] Clear old static_srmodels cache before loading from Flash\n");
    static_srmodels = NULL;
}
```

### 方法 3：使用 patch 命令

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
patch -p1 < patches/fix_vadnet1_cache.patch
```

## ✅ 验证补丁是否生效

查看设备日志，应该看到：

```
[PATCH] Clear old static_srmodels cache before loading from Flash
✅ ESP-SR 加载的模型数量: 3
   ESP-SR 模型 0: wn9_nihaoxiaozhi_tts
   ESP-SR 模型 1: nsnet2
   ESP-SR 模型 2: vadnet1_medium  ← 关键！
AFE Pipeline: [input] -> |NS(nsnet2)| -> |VAD(vadnet1_medium)| -> [output]
```

## 📊 当前配置（最佳音频质量）

- **降噪（NS）**: NSNet2 神经网络 ✅
- **人声检测（VAD）**: VADNet1 神经网络 ✅
- **自动增益（AGC）**: WebRTC 模式 (15dB)
- **Ringbuffer**: 500
- **AFE 优先级**: 4
- **AFE 核心**: CPU1

## 🐛 如果遇到问题

1. 检查补丁是否存在：
   ```bash
   grep -n "PATCH.*Clear old" managed_components/espressif__esp-sr/src/model_path.c
   ```

2. 如果没有输出，说明补丁被撤销了，重新运行：
   ```bash
   ./apply_vadnet1_fix.sh
   ```

3. 重新编译和烧录：
   ```bash
   idf.py build
   idf.py flash
   ```

---

**最后更新**: 2025-10-22

