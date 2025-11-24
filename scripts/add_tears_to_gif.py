#!/usr/bin/env python3
"""
Otto 机器人 - 在 GIF 上直接绘制泪滴
功能：修改 GIF 像素数据，在眼睛下方添加泪滴形状
作者：AI Assistant
日期：2025-10-17
"""

import os
import sys
import re
from pathlib import Path
from collections import Counter

def read_c_gif_file(c_file_path):
    """从 .c 文件读取 GIF 数据"""
    print(f"读取文件: {c_file_path}")
    
    with open(c_file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    map_marker = '_map[]'
    start_idx = content.find(map_marker)
    if start_idx == -1:
        raise ValueError("找不到 _map[] 数组定义")
    
    brace_start = content.find('{', start_idx)
    if brace_start == -1:
        raise ValueError("找不到数组开始")
    
    brace_count = 0
    brace_end = -1
    for i in range(brace_start, len(content)):
        if content[i] == '{':
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                brace_end = i
                break
    
    if brace_end == -1:
        raise ValueError("找不到数组结束")
    
    array_data = content[brace_start+1:brace_end]
    
    hex_values = []
    hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
    matches = hex_pattern.findall(array_data)
    
    for hex_str in matches:
        hex_values.append(int(hex_str, 16))
    
    gif_data = bytearray(hex_values)
    print(f"✓ 读取到 {len(gif_data)} 字节的 GIF 数据")
    
    return gif_data, content, brace_start, brace_end

def analyze_gif_structure(data):
    """分析 GIF 结构"""
    # GIF 头部
    signature = data[0:6].decode('ascii', errors='ignore')
    width = int.from_bytes(data[6:8], 'little')
    height = int.from_bytes(data[8:10], 'little')
    
    packed = data[10]
    has_color_table = (packed & 0x80) != 0
    color_table_size = 2 << (packed & 0x07)
    
    print(f"\nGIF 结构分析:")
    print(f"  版本: {signature}")
    print(f"  尺寸: {width} x {height}")
    print(f"  颜色表: {color_table_size} 色")
    
    return width, height, color_table_size

def find_tear_color_index(data):
    """找到或创建泪滴颜色的索引"""
    color_table_start = 13
    color_table_size = 256
    
    # 查找亮青色 #00FFFF
    TEAR_COLOR = (0x00, 0xFF, 0xFF)
    
    for i in range(color_table_size):
        idx = color_table_start + i * 3
        r, g, b = data[idx], data[idx+1], data[idx+2]
        if (r, g, b) == TEAR_COLOR:
            print(f"  找到泪滴颜色索引: {i}")
            return i
    
    # 如果没找到，使用第一个非黑非蓝的颜色并替换为泪滴色
    for i in range(color_table_size):
        idx = color_table_start + i * 3
        r, g, b = data[idx], data[idx+1], data[idx+2]
        # 跳过黑色和深蓝色
        if not (r == 0 and g == 0 and b == 0) and not (r == 0x1E and g == 0x90 and b == 0xFF):
            data[idx] = TEAR_COLOR[0]
            data[idx+1] = TEAR_COLOR[1]
            data[idx+2] = TEAR_COLOR[2]
            print(f"  创建泪滴颜色索引: {i} (替换 RGB({r},{g},{b}))")
            return i
    
    return 1  # 默认返回索引1

def add_tears_to_frames(data, tear_color_idx):
    """在 GIF 的所有帧中添加泪滴"""
    # GIF89a 格式：图像数据在扩展块之后
    # 我们需要找到图像描述符（0x2C）
    
    modifications = 0
    pos = 13 + 768  # 跳过颜色表
    
    print(f"\n正在添加泪滴效果...")
    print(f"  使用颜色索引: {tear_color_idx}")
    
    # 查找所有图像块
    while pos < len(data) - 10:
        if data[pos] == 0x21:  # 扩展块
            pos += 2
            while pos < len(data) and data[pos] != 0x00:
                block_size = data[pos]
                pos += block_size + 1
            pos += 1
        elif data[pos] == 0x2C:  # 图像描述符
            print(f"  找到图像块 @ 偏移 {pos}")
            
            # 读取图像位置和尺寸
            left = int.from_bytes(data[pos+1:pos+3], 'little')
            top = int.from_bytes(data[pos+3:pos+5], 'little')
            width = int.from_bytes(data[pos+5:pos+7], 'little')
            height = int.from_bytes(data[pos+7:pos+9], 'little')
            
            print(f"    位置: ({left}, {top}), 尺寸: {width}x{height}")
            
            # 跳过图像描述符和 LZW 压缩数据（这部分很复杂）
            pos += 10
            
            # LZW 数据以子块形式存储
            while pos < len(data) and data[pos] != 0x00:
                block_size = data[pos]
                pos += block_size + 1
            pos += 1
            
            modifications += 1
        elif data[pos] == 0x3B:  # GIF 终止符
            print(f"  到达 GIF 终止符 @ 偏移 {pos}")
            break
        else:
            pos += 1
    
    print(f"\n⚠️  警告: GIF 使用 LZW 压缩，无法直接修改像素")
    print(f"     需要解压 → 修改 → 重新压缩")
    
    return modifications

def create_tear_pattern():
    """创建泪滴图案（作为示例）"""
    # 泪滴形状（简化版）
    tear = """
      ..
     ....
    ......
    ......
     ....
      ..
       .
    """
    return tear

def main():
    script_dir = Path(__file__).parent
    
    # 使用黄色备份（原始版本）作为基础
    local_component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    sad_file = local_component_dir / 'sad.c'
    backup_file = local_component_dir / 'sad.c.yellow_backup'
    
    print("=" * 60)
    print("Otto 机器人 - 在 GIF 上直接绘制泪滴")
    print("功能：修改像素数据，添加真正的泪滴形状")
    print("=" * 60)
    print()
    
    if not backup_file.exists():
        print(f"❌ 错误：找不到原始备份文件")
        print(f"   需要: {backup_file}")
        return 1
    
    try:
        # 读取原始黄色版本
        data, original_content, brace_start, brace_end = read_c_gif_file(backup_file)
        
        # 分析 GIF 结构
        width, height, color_table_size = analyze_gif_structure(data)
        
        # 查找或创建泪滴颜色
        tear_color_idx = find_tear_color_index(data)
        
        # 尝试添加泪滴
        modifications = add_tears_to_frames(data, tear_color_idx)
        
        print()
        print("=" * 60)
        print("❌ 重要发现")
        print("=" * 60)
        print()
        print("GIF 文件使用 **LZW 压缩**，像素数据被压缩了。")
        print("要添加泪滴，需要：")
        print("  1. 解压 GIF")
        print("  2. 修改未压缩的像素数据")
        print("  3. 重新压缩为 GIF")
        print()
        print("这需要完整的 GIF 编解码器，超出了简单脚本的范围。")
        print()
        print("=" * 60)
        print("💡 可行的解决方案")
        print("=" * 60)
        print()
        print("**方案 A：使用 Python PIL/Pillow 库** ⭐⭐⭐⭐⭐")
        print("  我可以创建一个完整的脚本来：")
        print("  1. 使用 PIL 打开 GIF")
        print("  2. 在每一帧上绘制泪滴")
        print("  3. 保存为新的 GIF")
        print("  4. 转换回 C 数组")
        print()
        print("  需要安装: pip install Pillow")
        print()
        print("**方案 B：接受当前效果** ⭐⭐⭐⭐")
        print("  当前的蓝色眼睛 + 青色点缀已经是很好的效果")
        print()
        print("**方案 C：手工编辑 GIF** ⭐⭐⭐")
        print("  使用 Aseprite/GIMP 等工具手工绘制泪滴")
        print()
        print("=" * 60)
        print()
        print("🤔 您想使用哪个方案？")
        print()
        print("   输入 'A' - 我将创建完整的 PIL 脚本")
        print("   输入 'B' - 保持当前的蓝色眼睛效果")
        print("   输入 'C' - 我会提供手工编辑的详细教程")
        print()
        
        return 0
        
    except Exception as e:
        print(f"❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


