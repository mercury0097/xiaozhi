#!/usr/bin/env python3
"""
Otto 机器人悲伤表情 - 超明显泪滴效果
功能：创建非常明显的青蓝色泪滴，容易识别
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
    
    gif_data = bytes(hex_values)
    print(f"✓ 读取到 {len(gif_data)} 字节的 GIF 数据")
    
    return gif_data, content, brace_start, brace_end

def create_obvious_tears(gif_data):
    """创建超明显的泪滴效果"""
    data = bytearray(gif_data)
    
    if data[:6] not in [b'GIF89a', b'GIF87a']:
        raise ValueError("不是有效的 GIF 文件")
    
    print(f"GIF 版本: {data[:6].decode()}")
    
    packed_fields = data[10]
    has_global_color_table = (packed_fields & 0x80) != 0
    color_table_size = 2 << (packed_fields & 0x07)
    
    print(f"全局颜色表: {'是' if has_global_color_table else '否'}")
    print(f"颜色表大小: {color_table_size} 色")
    
    if not has_global_color_table:
        print("⚠️  没有全局颜色表")
        return data
    
    color_table_start = 13
    color_table_bytes = color_table_size * 3
    
    print(f"\n创建超明显泪滴效果...")
    modifications = 0
    
    # 定义明亮的青蓝色用于泪滴
    TEAR_CYAN = (0x00, 0xFF, 0xFF)  # 亮青色 #00FFFF (Cyan) - 非常明显！
    TEAR_LIGHT = (0x87, 0xCE, 0xFA)  # 浅天蓝 #87CEFA (LightSkyBlue)
    EYE_BLUE = (0x1E, 0x90, 0xFF)    # 深蓝眼睛 #1E90FF
    
    for i in range(0, color_table_bytes, 3):
        idx = color_table_start + i
        r, g, b = data[idx], data[idx+1], data[idx+2]
        
        # 保持纯黑色背景
        if r == 0 and g == 0 and b == 0:
            continue
        
        # 保持深蓝色眼睛
        if r == EYE_BLUE[0] and g == EYE_BLUE[1] and b == EYE_BLUE[2]:
            continue
        
        # 策略：将所有非黑、非眼睛的颜色都转换为明显的泪滴色
        
        # 计算亮度
        brightness = (r + g + b) / 3
        
        # 最亮的颜色（如果不是眼睛蓝）→ 亮青色泪滴
        if brightness > 150:
            data[idx] = TEAR_CYAN[0]
            data[idx+1] = TEAR_CYAN[1]
            data[idx+2] = TEAR_CYAN[2]
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB{TEAR_CYAN} [亮青泪滴] 💧💧")
        
        # 中等亮度 → 浅天蓝泪滴
        elif brightness > 80:
            data[idx] = TEAR_LIGHT[0]
            data[idx+1] = TEAR_LIGHT[1]
            data[idx+2] = TEAR_LIGHT[2]
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB{TEAR_LIGHT} [浅蓝泪滴] 💧")
        
        # 较暗 → 深青色泪痕
        elif brightness > 30:
            data[idx] = 0x00
            data[idx+1] = min(180, int(brightness * 3))
            data[idx+2] = min(200, int(brightness * 4))
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [深青泪痕] 💦")
        
        # 很暗 → 暗青色阴影
        else:
            data[idx] = 0x00
            data[idx+1] = min(100, int(brightness * 4))
            data[idx+2] = min(120, int(brightness * 5))
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [暗青阴影]")
    
    print(f"\n✓ 共修改了 {modifications} 个颜色")
    print(f"\n💧 超明显泪滴效果:")
    print(f"   💦 亮青色泪滴 (#00FFFF) - 非常醒目！")
    print(f"   💧 浅天蓝泪滴 (#87CEFA)")
    print(f"   🌊 深青色泪痕")
    print(f"   🔵 深蓝色眼睛 (#1E90FF)")
    
    return bytes(data)

def write_c_gif_file(gif_data, original_content, brace_start, brace_end, output_path):
    """将修改后的 GIF 数据写回 .c 文件"""
    prefix = original_content[:brace_start+1] + '\n'
    
    hex_lines = []
    for i in range(0, len(gif_data), 13):
        chunk = gif_data[i:i+13]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append(f'    {hex_str},')
    
    if hex_lines:
        hex_lines[-1] = hex_lines[-1].rstrip(',')
    
    suffix = '\n' + original_content[brace_end:]
    output_content = prefix + '\n'.join(hex_lines) + suffix
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(output_content)
    
    print(f"✓ 已保存到: {output_path}")

def main():
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
    print("Otto 机器人 - 超明显泪滴效果")
    print("功能：亮青色泪滴 (#00FFFF) 💧 非常醒目！")
    print("=" * 60)
    print()
    
    if not sad_file.exists():
        print(f"❌ 错误：找不到 sad.c 文件")
        return 1
    
    try:
        # 1. 读取当前文件
        print(f"处理: sad.c (创建超明显泪滴)")
        print("-" * 60)
        gif_data, original_content, brace_start, brace_end = read_c_gif_file(sad_file)
        
        # 2. 创建明显泪滴
        modified_data = create_obvious_tears(gif_data)
        
        # 3. 备份当前版本
        backup_path = sad_file.with_suffix('.c.subtle_tears')
        if not backup_path.exists():
            import shutil
            shutil.copy2(sad_file, backup_path)
            print(f"\n✓ 已备份柔和版本到: {backup_path.name}")
        
        # 4. 写入新版本
        write_c_gif_file(modified_data, original_content, brace_start, brace_end, sad_file)
        
        print()
        print("=" * 60)
        print("✅ 超明显泪滴效果已创建！")
        print("=" * 60)
        print()
        print("💧 效果说明：")
        print("   • 眼睛：深蓝色 🔵 (#1E90FF)")
        print("   • 泪滴：亮青色 💦 (#00FFFF) ← 非常明显！")
        print("   • 高光：浅天蓝 💧 (#87CEFA)")
        print("   • 泪痕：深青色渐变 🌊")
        print()
        print("📝 下一步操作：")
        print("   1. rm -rf build")
        print("   2. idf.py build")
        print("   3. idf.py flash")
        print()
        print("💡 如需恢复柔和版本:")
        print(f"   cp {backup_path} {sad_file}")
        print()
        
        return 0
        
    except Exception as e:
        print(f"❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


