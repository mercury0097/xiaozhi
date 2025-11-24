#!/bin/bash

# 🔧 恢复 VADNet1 和 NSNet2 神经网络模型

echo "🔍 检查模型文件..."

if [ ! -f "build/srmodels/srmodels.bin" ]; then
    echo "❌ 模型文件不存在，先编译项目："
    echo "   idf.py build"
    exit 1
fi

echo "✅ 找到模型文件: build/srmodels/srmodels.bin ($(ls -lh build/srmodels/srmodels.bin | awk '{print $5}'))"

# 列出模型内容
echo ""
echo "📦 包含的模型："
ls -1 build/srmodels/ | grep -v "srmodels.bin" | sed 's/^/   - /'

echo ""
echo "🔥 开始烧录模型到 model 分区 (0x800000)..."
echo ""

# 查找串口
PORT=$(ls /dev/tty.usbserial-* 2>/dev/null | head -1)
if [ -z "$PORT" ]; then
    PORT=$(ls /dev/tty.wchusbserial* 2>/dev/null | head -1)
fi

if [ -z "$PORT" ]; then
    echo "⚠️  未找到串口，请手动指定："
    echo "   ./恢复模型.sh /dev/tty.usbserial-XXXX"
    echo ""
    echo "或者使用 esptool.py："
    echo "   esptool.py -p /dev/tty.usbserial-XXXX write_flash 0x800000 build/srmodels/srmodels.bin"
    exit 1
fi

# 如果提供了参数，使用参数作为串口
if [ ! -z "$1" ]; then
    PORT=$1
fi

echo "📍 使用串口: $PORT"
echo ""

# 烧录模型分区
esptool.py -p $PORT write_flash 0x800000 build/srmodels/srmodels.bin

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 模型烧录成功！"
    echo ""
    echo "🎉 现在你的设备已恢复："
    echo "   ✅ VADNet1 (神经网络人声检测)"
    echo "   ✅ NSNet2 (神经网络降噪)"
    echo "   ✅ WakeNet9 (唤醒词检测)"
    echo ""
    echo "📊 重启后日志应该显示："
    echo "   I (xxxx) AfeAudioProcessor: 📦 Flash 中的模型数量: 3"
    echo "   I (xxxx) AfeAudioProcessor: ✅ ESP-SR 加载的模型数量: 3"
    echo "   I (xxxx) AfeAudioProcessor: ✅ VAD 人声检测: VADNet1 (神经网络)"
    echo "   I (xxxx) AfeAudioProcessor: ✅ 使用 ESP-SR 神经网络降噪: nsnet2"
    echo ""
    echo "🔄 重启设备以应用..."
else
    echo ""
    echo "❌ 烧录失败！请检查："
    echo "   1. 串口是否正确"
    echo "   2. 设备是否已连接"
    echo "   3. 是否有其他程序占用串口"
    echo ""
    echo "💡 可以手动烧录："
    echo "   esptool.py -p $PORT write_flash 0x800000 build/srmodels/srmodels.bin"
fi























