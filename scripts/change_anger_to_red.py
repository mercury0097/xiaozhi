#!/usr/bin/env python3
"""
Otto 机器人 - 将 anger 表情改为红色眼睛
功能：修改 GIF 颜色表，将眼睛从黄色改为红色
作者：AI Assistant
日期：2025-10-17
"""

import os
import sys
import re
from pathlib import Path
import shutil

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

def rgb_to_hex(r, g, b):
    """RGB 转十六进制字符串"""
    return f'#{r:02x}{g:02x}{b:02x}'

def modify_anger_colors(data):
    """修改 anger 表情颜色：黄色 → 红色"""
    color_table_start = 13
    color_table_size = 256
    
    modifications = 0
    color_stats = []
    
    print("\n颜色转换规则:")
    print("  🟡 黄色/白色 (亮区) → 🔴 纯红色 #FF0000")
    print("  🟡 金黄色 (中间区) → 🔴 深红色 #DC143C")
    print("  🟠 深黄色 (阴影区) → 🔴 暗红色 #8B0000")
    print("  ⬛ 黑色 (背景) → ⬛ 保持黑色 #000000")
    print()
    
    for i in range(color_table_size):
        idx = color_table_start + i * 3
        r = data[idx]
        g = data[idx + 1]
        b = data[idx + 2]
        
        old_rgb = (r, g, b)
        new_rgb = None
        color_type = ""
        
        # 黑色：保持不变
        if r == 0 and g == 0 and b == 0:
            continue
        
        # 判断是否是黄色系（从之前的 change_gif_eye_color.py 转换来的）
        # 或者是白色/灰色（原始眼睛颜色）
        
        # 非常亮的颜色（白色/亮黄色） → 纯红色
        if (r > 200 and g > 150) or (r > 200 and g > 200 and b > 200):
            new_rgb = (0xFF, 0x00, 0x00)  # 纯红色 #FF0000
            color_type = "🔴 纯红 (高光)"
        
        # 中等亮度的黄色 → 深红色
        elif r > 150 and g > 100 and b < 100:
            new_rgb = (0xDC, 0x14, 0x3C)  # Crimson 深红色 #DC143C
            color_type = "🔴 深红 (主体)"
        
        # 深黄色或橙色 → 暗红色
        elif r > 100 and g > 50 and b < 80:
            new_rgb = (0x8B, 0x00, 0x00)  # DarkRed 暗红色 #8B0000
            color_type = "🔴 暗红 (阴影)"
        
        # 灰色系（可能是眼睛轮廓）→ 深灰红色
        elif r > 50 and r < 150 and abs(r - g) < 30 and abs(r - b) < 30:
            # 保持灰度但加入红色调
            gray_level = (r + g + b) // 3
            new_rgb = (min(255, gray_level + 50), gray_level // 3, gray_level // 3)
            color_type = "🟥 灰红 (边缘)"
        
        # 其他非黑色 → 转换为红色调
        elif r + g + b > 30:  # 不是很深的颜色
            brightness = (r + g + b) // 3
            new_rgb = (min(255, brightness * 2), 0, 0)
            color_type = "🔴 红色调"
        
        if new_rgb:
            data[idx] = new_rgb[0]
            data[idx + 1] = new_rgb[1]
            data[idx + 2] = new_rgb[2]
            
            color_stats.append({
                'index': i,
                'old': old_rgb,
                'new': new_rgb,
                'type': color_type
            })
            
            modifications += 1
    
    print(f"\n✓ 修改了 {modifications} 种颜色")
    
    if color_stats:
        print("\n颜色转换详情 (前 10 个):")
        print("-" * 70)
        for stat in color_stats[:10]:
            old_hex = rgb_to_hex(*stat['old'])
            new_hex = rgb_to_hex(*stat['new'])
            print(f"  索引 {stat['index']:3d}: {old_hex} → {new_hex} {stat['type']}")
        if len(color_stats) > 10:
            print(f"  ... 还有 {len(color_stats) - 10} 种颜色")
    
    return modifications

def write_c_gif_file(c_file_path, gif_data, original_content, brace_start, brace_end):
    """将修改后的 GIF 数据写回 .c 文件"""
    print(f"\n写入文件: {c_file_path}")
    
    hex_lines = []
    for i in range(0, len(gif_data), 13):
        chunk = gif_data[i:i+13]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append(f'    {hex_str},')
    
    if hex_lines:
        hex_lines[-1] = hex_lines[-1].rstrip(',')
    
    new_content = (
        original_content[:brace_start+1] + '\n' +
        '\n'.join(hex_lines) + '\n' +
        original_content[brace_end:]
    )
    
    with open(c_file_path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print(f"✓ 已保存 {len(gif_data)} 字节")

def main():
    script_dir = Path(__file__).parent
    
    # 使用本地组件
    local_component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    anger_file = local_component_dir / 'anger.c'
    backup_file = local_component_dir / 'anger.c.yellow_backup'
    
    print("=" * 70)
    print("Otto 机器人 - 将 anger 表情改为红色眼睛 🔴")
    print("=" * 70)
    print()
    
    if not anger_file.exists():
        print(f"❌ 错误：找不到 anger.c 文件")
        print(f"   路径: {anger_file}")
        return 1
    
    try:
        # 备份原始文件
        if not backup_file.exists():
            shutil.copy2(anger_file, backup_file)
            print(f"✓ 已备份原始文件: {backup_file.name}")
        else:
            print(f"✓ 备份文件已存在: {backup_file.name}")
        
        # 读取 GIF 数据
        data, original_content, brace_start, brace_end = read_c_gif_file(anger_file)
        
        # 修改颜色
        modifications = modify_anger_colors(data)
        
        if modifications == 0:
            print("\n⚠️  警告：没有修改任何颜色")
            print("   anger.c 可能已经是红色，或颜色格式不符合预期")
            return 0
        
        # 写回文件
        write_c_gif_file(anger_file, data, original_content, brace_start, brace_end)
        
        print()
        print("=" * 70)
        print("✅ 成功！anger 表情已改为红色眼睛 🔴")
        print("=" * 70)
        print()
        print("🎨 颜色方案:")
        print("  • 眼睛高光: 🔴 纯红色 #FF0000")
        print("  • 眼睛主体: 🔴 深红色 #DC143C (Crimson)")
        print("  • 眼睛阴影: 🔴 暗红色 #8B0000 (DarkRed)")
        print("  • 背景区域: ⬛ 纯黑色 #000000")
        print()
        print("📝 下一步:")
        print("  1. rm -rf build")
        print("  2. idf.py build")
        print("  3. idf.py flash")
        print()
        print("🎭 所有表情颜色:")
        print("  • neutral/happy: 🟡 黄色")
        print("  • sad: 🔵 蓝色")
        print("  • anger: 🔴 红色 ★ 新设置")
        print()
        
        return 0
        
    except Exception as e:
        print(f"\n❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


