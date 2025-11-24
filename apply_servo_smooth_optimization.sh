#!/bin/bash

# Otto舵机平滑度优化应用脚本
# 用途：将优化版本的Oscillator应用到项目中

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OTTO_DIR="$SCRIPT_DIR/main/boards/otto-robot"

echo "======================================"
echo "  Otto舵机平滑度优化应用脚本"
echo "======================================"
echo ""

# 检查文件是否存在
if [ ! -f "$OTTO_DIR/oscillator_smooth.h" ] || [ ! -f "$OTTO_DIR/oscillator_smooth.cc" ]; then
    echo "❌ 错误：找不到优化文件"
    echo "   请确保 oscillator_smooth.h 和 oscillator_smooth.cc 存在"
    exit 1
fi

echo "📋 优化方案："
echo "   1. 采样频率：30ms → 15ms (提升100%)"
echo "   2. 添加缓动函数（S型加速/减速）"
echo "   3. 优化舵机脉宽计算精度"
echo "   4. 支持可调平滑度等级"
echo ""

# 提供选项
echo "请选择应用方式："
echo "  [1] 备份原文件并替换（推荐）"
echo "  [2] 仅创建备份，手动替换"
echo "  [3] 取消操作"
echo ""
read -p "请输入选项 [1-3]: " choice

case $choice in
    1)
        echo ""
        echo "🔄 开始应用优化..."
        
        # 备份原文件
        if [ -f "$OTTO_DIR/oscillator.h" ]; then
            echo "📦 备份原文件: oscillator.h → oscillator_original.h.bak"
            cp "$OTTO_DIR/oscillator.h" "$OTTO_DIR/oscillator_original.h.bak"
        fi
        
        if [ -f "$OTTO_DIR/oscillator.cc" ]; then
            echo "📦 备份原文件: oscillator.cc → oscillator_original.cc.bak"
            cp "$OTTO_DIR/oscillator.cc" "$OTTO_DIR/oscillator_original.cc.bak"
        fi
        
        # 替换为优化版本
        echo "🔄 替换为优化版本..."
        cp "$OTTO_DIR/oscillator_smooth.h" "$OTTO_DIR/oscillator.h"
        cp "$OTTO_DIR/oscillator_smooth.cc" "$OTTO_DIR/oscillator.cc"
        
        echo ""
        echo "✅ 优化已应用！"
        echo ""
        echo "📌 下一步："
        echo "   1. 运行 ./compile.sh 重新编译"
        echo "   2. 烧录到设备"
        echo "   3. 测试行走平滑度"
        echo ""
        echo "💡 如需恢复原版本，运行："
        echo "   cd $OTTO_DIR"
        echo "   cp oscillator_original.h.bak oscillator.h"
        echo "   cp oscillator_original.cc.bak oscillator.cc"
        echo ""
        ;;
        
    2)
        echo ""
        echo "📦 创建备份..."
        
        if [ -f "$OTTO_DIR/oscillator.h" ]; then
            cp "$OTTO_DIR/oscillator.h" "$OTTO_DIR/oscillator_original.h.bak"
            echo "✅ 已备份: oscillator_original.h.bak"
        fi
        
        if [ -f "$OTTO_DIR/oscillator.cc" ]; then
            cp "$OTTO_DIR/oscillator.cc" "$OTTO_DIR/oscillator_original.cc.bak"
            echo "✅ 已备份: oscillator_original.cc.bak"
        fi
        
        echo ""
        echo "📌 请手动执行以下命令应用优化："
        echo "   cd $OTTO_DIR"
        echo "   cp oscillator_smooth.h oscillator.h"
        echo "   cp oscillator_smooth.cc oscillator.cc"
        echo ""
        ;;
        
    3)
        echo ""
        echo "❌ 操作已取消"
        exit 0
        ;;
        
    *)
        echo ""
        echo "❌ 无效的选项"
        exit 1
        ;;
esac

echo "======================================"
echo "  优化文档："
echo "  查看 Otto_舵机平滑度优化方案.md"
echo "======================================"






















