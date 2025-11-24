#!/usr/bin/env python3
"""
Otto 机器人悲伤表情 - 蓝色眼睛 + 泪滴效果
功能：将 sad 表情的眼睛改为蓝色，泪滴改为浅蓝色
作者：AI Assistant
日期：2025-10-17
"""

import os
import sys
import re
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
    
    # 解析十六进制数据
    hex_values = []
    hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
    matches = hex_pattern.findall(array_data)
    
    for hex_str in matches:
        hex_values.append(int(hex_str, 16))
    
    gif_data = bytes(hex_values)
    print(f"✓ 读取到 {len(gif_data)} 字节的 GIF 数据")
    
    return gif_data, content, brace_start, brace_end

def modify_sad_colors(gif_data):
    """修改悲伤表情颜色：黄色→蓝色 + 泪滴效果"""
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
    color_table_bytes = color_table_size * 3
    
    print(f"\n修改颜色表 - 黄色眼睛 → 蓝色泪眼...")
    modifications = 0
    
    for i in range(0, color_table_bytes, 3):
        idx = color_table_start + i
        r, g, b = data[idx], data[idx+1], data[idx+2]
        
        # 保持纯黑色背景
        if r == 0 and g == 0 and b == 0:
            continue  # 黑色保持不变
        
        # 将黄色系改为蓝色系
        # 检测黄色：R 和 G 较高，B 较低
        if r > 50 and g > 40 and b < 50:
            # 这是黄色，将其转换为蓝色
            # 计算原始亮度
            brightness = (r + g) / 2
            
            # 最亮的黄色 → 深蓝色（眼睛主体）
            if brightness > 200:
                data[idx] = 0x1E      # R = 30
                data[idx+1] = 0x90    # G = 144
                data[idx+2] = 0xFF    # B = 255
                modifications += 1
                print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB(30,144,255) [深蓝眼睛] 💙")
            
            # 亮黄色 → 天蓝色（高光）
            elif brightness > 150:
                data[idx] = 0x87      # R = 135
                data[idx+1] = 0xCE    # G = 206
                data[idx+2] = 0xEB    # B = 235
                modifications += 1
                print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB(135,206,235) [天蓝高光] 💧")
            
            # 中等黄色 → 青蓝色（泪滴效果）
            elif brightness > 100:
                data[idx] = 0x00      # R = 0
                data[idx+1] = 0xCE    # G = 206
                data[idx+2] = 0xD1    # B = 209
                modifications += 1
                print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB(0,206,209) [青蓝泪滴] 💧💧")
            
            # 暗黄色 → 深青色（阴影/泪痕）
            else:
                # 根据亮度调整青色深度
                blue_intensity = int(brightness * 1.8)
                data[idx] = max(0, int(brightness * 0.2))
                data[idx+1] = min(180, int(brightness * 1.2))
                data[idx+2] = min(220, blue_intensity)
                modifications += 1
                print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [深青阴影] 💦")
    
    print(f"\n✓ 共修改了 {modifications} 个颜色")
    print(f"\n💙 悲伤表情特效:")
    print(f"   🔵 深蓝色眼睛 (#1E90FF)")
    print(f"   💧 天蓝色高光 (#87CEEB)")
    print(f"   💦 青蓝色泪滴 (#00CED1)")
    print(f"   ⬛ 纯黑色背景 (#000000)")
    
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
    local_component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    managed_component_dir = script_dir.parent / 'managed_components' / 'txp666__otto-emoji-gif-component' / 'src'
    
    if local_component_dir.exists():
        component_dir = local_component_dir
        print(f"✅ 找到本地组件: {component_dir}")
    elif managed_component_dir.exists():
        component_dir = managed_component_dir
        print(f"⚠️  使用 managed_components: {component_dir}")
    else:
        print(f"❌ 错误：找不到组件目录")
        return 1
    
    sad_file = component_dir / 'sad.c'
    
    print("=" * 60)
    print("Otto 机器人 - 悲伤表情蓝色特效")
    print("功能：蓝色眼睛 💙 + 泪滴效果 💧")
    print("=" * 60)
    print()
    
    if not sad_file.exists():
        print(f"❌ 错误：找不到 sad.c 文件")
        return 1
    
    try:
        # 1. 读取原始 C 文件
        print(f"处理: sad.c")
        print("-" * 60)
        gif_data, original_content, brace_start, brace_end = read_c_gif_file(sad_file)
        
        # 2. 修改颜色
        modified_data = modify_sad_colors(gif_data)
        
        # 3. 备份原文件
        backup_path = sad_file.with_suffix('.c.yellow_backup')
        if not backup_path.exists():
            import shutil
            shutil.copy2(sad_file, backup_path)
            print(f"\n✓ 已备份黄色版本到: {backup_path.name}")
        
        # 4. 写入修改后的文件
        write_c_gif_file(modified_data, original_content, brace_start, brace_end, sad_file)
        
        print()
        print("=" * 60)
        print("✅ 悲伤表情修改完成！")
        print("=" * 60)
        print()
        print("💙 效果预览：")
        print("   • 眼睛：深蓝色 🔵")
        print("   • 高光：天蓝色 💧")
        print("   • 泪滴：青蓝色 💦")
        print("   • 背景：纯黑色 ⬛")
        print()
        print("📝 下一步操作：")
        print("   1. 删除 build 目录: rm -rf build")
        print("   2. 重新编译: idf.py build")
        print("   3. 烧录: idf.py flash")
        print()
        print("💡 如需恢复黄色眼睛:")
        print(f"   cp {backup_path} {sad_file}")
        print()
        
        return 0
        
    except Exception as e:
        print(f"❌ 处理 sad.c 时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())

