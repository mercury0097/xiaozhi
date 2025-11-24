#!/bin/bash

echo "🔥 完整烧录 - 包含模型分区"
echo "================================"
echo ""

# 设置串口
PORT="/dev/tty.usbmodem101"
echo "📍 串口: $PORT"
echo ""

# 检查必要文件
echo "🔍 检查文件..."
FILES=(
    "build/bootloader/bootloader.bin"
    "build/partition_table/partition-table.bin"
    "build/xiaozhi.bin"
    "build/srmodels/srmodels.bin"
    "build/ota_data_initial.bin"
    "build/generated_assets.bin"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        size=$(ls -lh "$file" | awk '{print $5}')
        echo "  ✅ $file ($size)"
    else
        echo "  ❌ $file - 缺失！"
        exit 1
    fi
done

echo ""
echo "✅ 所有文件就绪"
echo ""
echo "🎯 即将烧录以下分区："
echo "  0x0      - bootloader"
echo "  0x8000   - partition table"
echo "  0x10000  - app (固件)"
echo "  0x600000 - assets"
echo "  0x700000 - ota_data"
echo "  0x800000 - model (VADNet1 + NSNet2) ⭐"
echo ""

# 提示用户
read -p "按回车继续烧录，或 Ctrl+C 取消..."

echo ""
echo "🔄 开始烧录..."
echo ""

# 方法1：尝试使用 idf.py
if command -v idf.py &> /dev/null; then
    echo "📌 使用 idf.py flash (推荐方式)"
    idf.py -p $PORT flash
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✅ 烧录成功！"
        echo ""
        echo "🎉 已烧录："
        echo "  ✅ VADNet1 (神经网络VAD)"
        echo "  ✅ NSNet2 (神经网络降噪)"
        echo "  ✅ WakeNet9 (唤醒词)"
        echo ""
        echo "📊 重启后应该看到："
        echo "  I (xxxx) 📦 Flash 中的模型数量: 3"
        echo "  I (xxxx) ✅ ESP-SR 加载的模型数量: 3"
        echo "  I (xxxx) ✅ VAD 人声检测: VADNet1 (神经网络)"
        echo "  I (xxxx) ✅ 使用 ESP-SR 神经网络降噪: nsnet2"
        echo ""
        exit 0
    fi
fi

# 方法2：手动指定所有分区
echo "📌 使用手动烧录方式"
echo ""

# 查找 python3 esptool
PYTHON=""
if command -v python3 &> /dev/null; then
    PYTHON="python3"
elif command -v python &> /dev/null; then
    PYTHON="python"
fi

if [ -z "$PYTHON" ]; then
    echo "❌ 未找到 Python，无法继续"
    echo ""
    echo "💡 请在 VSCode 的 ESP-IDF 终端中运行："
    echo "   idf.py -p $PORT flash"
    exit 1
fi

# 检查 esptool
if ! $PYTHON -m esptool version &> /dev/null; then
    echo "❌ esptool 未安装"
    echo ""
    echo "💡 请在 VSCode 的 ESP-IDF 终端中运行："
    echo "   idf.py -p $PORT flash"
    exit 1
fi

# 手动烧录所有分区
echo "开始烧录各个分区..."
echo ""

$PYTHON -m esptool -p $PORT -b 460800 --before default_reset --after hard_reset \
    --chip esp32s3 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
    0x0 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x10000 build/xiaozhi.bin \
    0x600000 build/generated_assets.bin \
    0x700000 build/ota_data_initial.bin \
    0x800000 build/srmodels/srmodels.bin

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 烧录成功！"
else
    echo ""
    echo "❌ 烧录失败"
    echo ""
    echo "💡 请尝试："
    echo "   1. 在 VSCode 中使用 ESP-IDF Flash 功能"
    echo "   2. 或在 ESP-IDF 终端中运行: idf.py -p $PORT flash"
    exit 1
fi























