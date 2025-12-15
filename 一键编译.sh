#!/bin/bash

# 小智ESP32固件一键编译脚本
# 板子型号: Palqiqi (帕奇奇机器人)
# 服务器地址: http://192.168.1.151:8003/xiaozhi/ota/

echo "=========================================="
echo "  小智ESP32固件一键编译脚本"
echo "  板子型号: Palqiqi (帕奇奇机器人)"
echo "  服务器地址: http://192.168.1.151:8003/xiaozhi/ota/"
echo "=========================================="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "📍 当前目录: $SCRIPT_DIR"
echo ""

# 检查ESP-IDF环境
if ! command -v idf.py &> /dev/null; then
    echo "❌ 错误: 未找到 idf.py 命令"
    echo "请先安装并配置 ESP-IDF 环境"
    echo "参考: https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/"
    exit 1
fi

echo "✅ ESP-IDF 环境检测成功"
idf.py --version
echo ""

# 设置编译目标
echo "🎯 设置编译目标为 ESP32-S3..."
idf.py set-target esp32s3
if [ $? -ne 0 ]; then
    echo "❌ 设置编译目标失败"
    exit 1
fi
echo ""

# 清理之前的编译
echo "🧹 清理之前的编译文件..."
idf.py fullclean
echo ""

# 开始编译
echo "🔨 开始编译固件..."
echo "提示: 首次编译可能需要较长时间，请耐心等待..."
echo ""
idf.py build

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ 编译失败，请检查错误信息"
    exit 1
fi

echo ""
echo "=========================================="
echo "✅ 固件编译成功！"
echo "=========================================="
echo ""

# 打包固件
echo "📦 打包固件..."
cd scripts
python release.py
cd ..

if [ -f "build/merged-binary.bin" ]; then
    echo ""
    echo "=========================================="
    echo "✅ 固件打包成功！"
    echo "=========================================="
    echo ""
    echo "📁 固件文件位置:"
    echo "   $SCRIPT_DIR/build/merged-binary.bin"
    echo ""
    echo "📏 文件大小:"
    ls -lh build/merged-binary.bin | awk '{print "   " $5}'
    echo ""
    echo "🔥 烧录方法:"
    echo ""
    echo "方法一: USB直接烧录（推荐）"
    echo "  1. 用USB线连接Palqiqi板子到电脑"
    echo "  2. 运行命令: idf.py -p /dev/cu.usbserial* flash monitor"
    echo ""
    echo "方法二: 网页烧录工具"
    echo "  1. 用Chrome浏览器打开: https://espressif.github.io/esp-launchpad/"
    echo "  2. 上传文件: build/merged-binary.bin"
    echo "  3. 按照提示烧录"
    echo ""
    echo "📱 WiFi配网:"
    echo "  烧录后，板子会创建热点 'xiaozhi-XXXXXX'"
    echo "  连接热点后访问: http://192.168.4.1"
    echo "  WiFi名称: 2223"
    echo "  WiFi密码: GYXZXY88"
    echo ""
    echo "🎉 配置完成！祝使用愉快！"
    echo "=========================================="
else
    echo ""
    echo "⚠️  警告: 未找到打包后的固件文件"
    echo "但编译可能已成功，请检查 build 目录"
fi

