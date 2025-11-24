#!/usr/bin/env python3
"""
Otto 机器人 GIF 表情颜色转换工具
功能：将白色眼睛改为黄色，保持黑色背景
作者：AI Assistant
日期：2025-10-17
"""

import os
import sys
import struct
from pathlib import Path

def read_c_gif_file(c_file_path):
    """从 .c 文件读取 GIF 数据"""
    print(f"读取文件: {c_file_path}")
    
    with open(c_file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 查找数组定义（查找 "_map[]" 数组）
    map_marker = '_map[]'
    start_idx = content.find(map_marker)
    if start_idx == -1:
        raise ValueError("找不到 _map[] 数组定义")
    
    # 找到数组数据的开始 {
    brace_start = content.find('{', start_idx)
    if brace_start == -1:
        raise ValueError("找不到数组开始")
    
    # 找到对应的 }; (数组结尾)
    # 需要匹配括号来找到正确的结尾
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
    
    # 解析十六进制数据（更健壮的解析）
    hex_values = []
    import re
    hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
    matches = hex_pattern.findall(array_data)
    
    for hex_str in matches:
        hex_values.append(int(hex_str, 16))
    
    gif_data = bytes(hex_values)
    print(f"✓ 读取到 {len(gif_data)} 字节的 GIF 数据")
    
    return gif_data, content, brace_start, brace_end

def modify_gif_colors(gif_data):
    """修改 GIF 颜色表：将白色改为黄色，深灰色改为纯黑"""
    # GIF 文件格式：
    # - 头部：6 字节 "GIF89a" 或 "GIF87a"
    # - 逻辑屏幕描述符：7 字节
    # - 全局颜色表（如果有）
    
    data = bytearray(gif_data)
    
    # 检查 GIF 签名
    if data[:6] not in [b'GIF89a', b'GIF87a']:
        raise ValueError("不是有效的 GIF 文件")
    
    print(f"GIF 版本: {data[:6].decode()}")
    
    # 读取逻辑屏幕描述符
    packed_fields = data[10]
    has_global_color_table = (packed_fields & 0x80) != 0
    color_table_size = 2 << (packed_fields & 0x07)
    
    print(f"全局颜色表: {'是' if has_global_color_table else '否'}")
    print(f"颜色表大小: {color_table_size} 色")
    
    if not has_global_color_table:
        print("⚠️  没有全局颜色表，可能无法修改颜色")
        return data
    
    # 全局颜色表从字节 13 开始
    color_table_start = 13
    color_table_bytes = color_table_size * 3  # 每个颜色 3 字节 (R, G, B)
    
    print(f"\n修改颜色表...")
    modifications = 0
    
    for i in range(0, color_table_bytes, 3):
        idx = color_table_start + i
        r, g, b = data[idx], data[idx+1], data[idx+2]
        
        # 将深灰色（接近黑色）改为纯黑色
        if r < 10 and g < 10 and b < 10 and (r > 0 or g > 0 or b > 0):
            data[idx] = 0
            data[idx+1] = 0
            data[idx+2] = 0
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB(0,0,0) [纯黑]")
        
        # 将白色/亮灰色改为亮黄色
        elif r > 200 and g > 200 and b > 200:
            # 亮黄色 #FFD700
            data[idx] = 0xFF      # R
            data[idx+1] = 0xD7    # G
            data[idx+2] = 0x00    # B
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB(255,215,0) [亮黄]")
        
        # 将中等灰色改为暗黄色
        elif 50 < r < 200 and 50 < g < 200 and 50 < b < 200 and abs(r-g) < 20 and abs(g-b) < 20:
            # 根据亮度调整黄色深度
            brightness = (r + g + b) / 3
            yellow_intensity = int(brightness * 1.2)  # 稍微增强
            data[idx] = min(255, yellow_intensity)
            data[idx+1] = min(200, int(yellow_intensity * 0.84))
            data[idx+2] = 0
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},0) [暗黄]")
    
    print(f"\n✓ 共修改了 {modifications} 个颜色")
    return bytes(data)

def write_c_gif_file(gif_data, original_content, brace_start, brace_end, output_path):
    """将修改后的 GIF 数据写回 .c 文件"""
    # 保留文件开头（直到数组数据开始）
    prefix = original_content[:brace_start+1] + '\n'
    
    # 生成新的十六进制数据
    hex_lines = []
    for i in range(0, len(gif_data), 13):
        chunk = gif_data[i:i+13]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append(f'    {hex_str},')
    
    # 最后一行去掉逗号
    if hex_lines:
        hex_lines[-1] = hex_lines[-1].rstrip(',')
    
    # 保留文件末尾（从数组结束后）
    suffix = '\n' + original_content[brace_end:]
    
    # 拼接完整文件
    output_content = prefix + '\n'.join(hex_lines) + suffix
    
    # 写入文件
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(output_content)
    
    print(f"✓ 已保存到: {output_path}")

def main():
    # 设置路径
    script_dir = Path(__file__).parent
    # 优先修改本地组件，如果不存在则修改 managed_components
    local_component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    managed_component_dir = script_dir.parent / 'managed_components' / 'txp666__otto-emoji-gif-component' / 'src'
    
    if local_component_dir.exists():
        component_dir = local_component_dir
        print(f"✅ 找到本地组件，将修改: {component_dir}")
    elif managed_component_dir.exists():
        component_dir = managed_component_dir
        print(f"⚠️  未找到本地组件，将修改 managed_components: {component_dir}")
    else:
        print(f"❌ 错误：找不到任何组件目录")
        return 1
    
    # 要处理的 GIF 文件列表
    gif_files = [
        'staticstate.c',  # 静态表情（最常用）
        'happy.c',
        'sad.c',
        'anger.c',
        'scare.c',
        'buxue.c',
    ]
    
    print("=" * 60)
    print("Otto 机器人 GIF 表情颜色转换工具")
    print("功能：白色眼睛 → 亮黄色，黑色背景保持")
    print("=" * 60)
    print()
    
    # 检查目录是否存在
    if not component_dir.exists():
        print(f"❌ 错误：找不到组件目录")
        print(f"   预期路径: {component_dir}")
        return 1
    
    # 处理每个 GIF 文件
    total_processed = 0
    for gif_file in gif_files:
        c_file_path = component_dir / gif_file
        
        if not c_file_path.exists():
            print(f"⚠️  跳过: {gif_file} (文件不存在)")
            continue
        
        print(f"\n{'='*60}")
        print(f"处理: {gif_file}")
        print('='*60)
        
        try:
            # 1. 读取原始 C 文件
            gif_data, original_content, brace_start, brace_end = read_c_gif_file(c_file_path)
            
            # 2. 修改颜色
            modified_data = modify_gif_colors(gif_data)
            
            # 3. 备份原文件
            backup_path = c_file_path.with_suffix('.c.backup')
            if not backup_path.exists():
                import shutil
                shutil.copy2(c_file_path, backup_path)
                print(f"✓ 已备份原文件到: {backup_path.name}")
            
            # 4. 写入修改后的文件
            write_c_gif_file(modified_data, original_content, brace_start, brace_end, c_file_path)
            
            total_processed += 1
            print(f"✅ {gif_file} 处理完成！")
            
        except Exception as e:
            print(f"❌ 处理 {gif_file} 时出错: {e}")
            import traceback
            traceback.print_exc()
            continue
    
    print(f"\n{'='*60}")
    print(f"处理完成！共成功处理 {total_processed}/{len(gif_files)} 个文件")
    print('='*60)
    print()
    print("📝 下一步操作：")
    print("   1. 重新编译项目: idf.py build")
    print("   2. 烧录到设备: idf.py flash")
    print("   3. 查看效果！")
    print()
    print("💡 如需恢复原始文件，请使用 .backup 文件")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

