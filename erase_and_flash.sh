#!/bin/bash
echo "🔥 步骤 1: 完全擦除 Flash..."
idf.py erase-flash

echo ""
echo "⏳ 等待 3 秒..."
sleep 3

echo ""
echo "🚀 步骤 2: 重新烧录所有分区..."
idf.py flash

echo ""
echo "✅ 完成！现在启动 Monitor..."
idf.py monitor
