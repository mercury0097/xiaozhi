#!/bin/bash

# ESP-IDF 编译脚本
# 使用方法: ./compile.sh

set -e

echo "🔧 开始编译 xiaozhi-esp32..."

# 尝试不同的 ESP-IDF 路径
if [ -f "$HOME/.espressif/frameworks/esp-idf-v5.5/export.sh" ]; then
    echo "📦 找到 ESP-IDF v5.5"
    source "$HOME/.espressif/frameworks/esp-idf-v5.5/export.sh"
elif [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    echo "📦 找到 ESP-IDF (默认位置)"
    source "$HOME/esp/esp-idf/export.sh"
elif [ -f "/opt/esp-idf/export.sh" ]; then
    echo "📦 找到 ESP-IDF (/opt)"
    source "/opt/esp-idf/export.sh"
else
    echo "❌ 未找到 ESP-IDF，请手动设置环境"
    echo "   提示：运行 'source ~/esp/esp-idf/export.sh' 或类似命令"
    exit 1
fi

# 编译
idf.py build

echo "✅ 编译完成！"
echo ""
echo "📝 下一步："
echo "   1. 烧录: idf.py flash"
echo "   2. 监控: idf.py monitor"
echo "   3. 或者: idf.py flash monitor"

