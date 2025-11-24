#!/bin/bash
# 手动烧录 model 分区（包含 VADNet1）

echo "🚀 开始烧录 model 分区..."
echo "📦 源文件: build/srmodels/srmodels.bin"
echo "📍 目标地址: 0x800000"
echo ""

esptool.py --chip esp32s3 --port /dev/tty.usbmodem101 --baud 460800 write_flash 0x800000 build/srmodels/srmodels.bin

echo ""
echo "✅ 烧录完成！现在重启设备并监控日志"

