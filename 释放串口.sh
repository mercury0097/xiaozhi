#!/bin/bash
# 🔓 释放 ESP32 串口占用
# 使用方法：bash 释放串口.sh

SERIAL_PORT="/dev/tty.usbmodem101"

echo "🔍 正在检查串口占用情况..."
echo ""

# 查找占用串口的进程
PIDS=$(lsof 2>/dev/null | grep "$SERIAL_PORT" | awk '{print $2}' | sort -u)

if [ -z "$PIDS" ]; then
    echo "✅ 串口未被占用，可以正常烧录"
    exit 0
fi

echo "⚠️  发现以下进程占用串口："
echo ""

# 显示详细信息
lsof 2>/dev/null | grep "$SERIAL_PORT" | while read line; do
    PID=$(echo "$line" | awk '{print $2}')
    CMD=$(echo "$line" | awk '{for(i=1;i<=NF;i++) if($i ~ /Python|monitor|esptool/) print $i}')
    echo "  PID: $PID - 命令: $CMD"
done

echo ""
echo "🔨 正在关闭这些进程..."

# 关闭所有占用串口的进程
for PID in $PIDS; do
    kill -9 "$PID" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  ✅ 已关闭进程 $PID"
    else
        echo "  ❌ 无法关闭进程 $PID（可能需要 sudo）"
    fi
done

echo ""
echo "🎉 串口已释放，现在可以烧录了！"


