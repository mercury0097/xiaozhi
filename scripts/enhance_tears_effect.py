#!/usr/bin/env python3
"""
Otto 机器人悲伤表情 - 增强泪滴效果
功能：在蓝色眼睛基础上，将暗色区域转换为浅蓝色泪痕
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

def enhance_tears(gif_data):
    """增强泪滴效果：将暗色区域转换为浅蓝色泪痕"""
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
    
    print(f"\n增强泪滴效果 - 创建浅蓝色泪痕...")
    modifications = 0
    
    for i in range(0, color_table_bytes, 3):
        idx = color_table_start + i
        r, g, b = data[idx], data[idx+1], data[idx+2]
        
        # 保持纯黑色和深蓝色不变
        if (r == 0 and g == 0 and b == 0) or (r == 0x1E and g == 0x90 and b == 0xFF):
            continue
        
        # 将暗色区域转换为浅蓝色泪痕
        # 暗灰色 (10-50): 转换为浅蓝色（泪痕）
        if 10 <= r <= 50 and 10 <= g <= 50 and 10 <= b <= 50:
            # 浅蓝色泪痕 - 半透明感觉
            brightness = (r + g + b) / 3
            data[idx] = min(100, int(brightness * 2.5))     # R
            data[idx+1] = min(180, int(brightness * 4.5))   # G
            data[idx+2] = min(220, int(brightness * 5.5))   # B
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [浅蓝泪痕] 💧")
        
        # 稍亮的暗色 (51-99): 转换为青蓝色（泪滴扩散）
        elif 51 <= r <= 99 and 51 <= g <= 99 and 51 <= b <= 99:
            brightness = (r + g + b) / 3
            data[idx] = min(80, int(brightness * 1.8))
            data[idx+1] = min(160, int(brightness * 3.5))
            data[idx+2] = min(200, int(brightness * 4.5))
            modifications += 1
            print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [青蓝扩散] 💦")
        
        # 检查是否有天蓝色或青蓝色（保持）
        elif (r == 0x87 and g == 0xCE and b == 0xEB) or (r == 0x00 and g == 0xCE and b == 0xD1):
            continue  # 已经是泪滴色，保持
        
        # 其他深色区域: 稍微增加蓝色分量
        elif r < 10 or g < 10 or b < 10:
            if r > 0 or g > 0 or b > 0:
                # 深色但不是纯黑 - 添加微弱蓝色
                data[idx] = max(0, r)
                data[idx+1] = max(0, g)
                data[idx+2] = min(50, b + 30)  # 增加蓝色分量
                modifications += 1
                print(f"  色表[{i//3}]: RGB({r},{g},{b}) -> RGB({data[idx]},{data[idx+1]},{data[idx+2]}) [深蓝底色]")
    
    print(f"\n✓ 共修改了 {modifications} 个颜色")
    print(f"\n💧 泪痕效果已增强:")
    print(f"   💙 深蓝色眼睛 (#1E90FF)")
    print(f"   💧 浅蓝色泪痕 (暗色区域)")
    print(f"   💦 青蓝色扩散 (过渡区域)")
    print(f"   🌊 微弱蓝光 (深色区域)")
    
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
    print("Otto 机器人 - 增强悲伤表情泪滴效果")
    print("功能：将暗色区域转换为浅蓝色泪痕 💧💦")
    print("=" * 60)
    print()
    
    if not sad_file.exists():
        print(f"❌ 错误：找不到 sad.c 文件")
        return 1
    
    try:
        # 1. 读取当前文件（蓝色眼睛版本）
        print(f"处理: sad.c (增强泪滴效果)")
        print("-" * 60)
        gif_data, original_content, brace_start, brace_end = read_c_gif_file(sad_file)
        
        # 2. 增强泪滴效果
        modified_data = enhance_tears(gif_data)
        
        # 3. 备份当前版本
        backup_path = sad_file.with_suffix('.c.blue_simple')
        if not backup_path.exists():
            import shutil
            shutil.copy2(sad_file, backup_path)
            print(f"\n✓ 已备份简单蓝色版本到: {backup_path.name}")
        
        # 4. 写入增强版本
        write_c_gif_file(modified_data, original_content, brace_start, brace_end, sad_file)
        
        print()
        print("=" * 60)
        print("✅ 泪滴效果已增强！")
        print("=" * 60)
        print()
        print("💧 效果说明：")
        print("   • 眼睛：深蓝色 🔵")
        print("   • 泪痕：浅蓝色（暗色区域）💧")
        print("   • 扩散：青蓝色（过渡区域）💦")
        print("   • 底色：微弱蓝光 🌊")
        print()
        print("📝 下一步操作：")
        print("   1. 删除 build: rm -rf build")
        print("   2. 重新编译: idf.py build")
        print("   3. 烧录: idf.py flash")
        print()
        print("💡 如需恢复简单蓝色版本:")
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


