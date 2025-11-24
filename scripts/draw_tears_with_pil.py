#!/usr/bin/env python3
"""
Otto 机器人 - 使用 PIL 在 GIF 上绘制泪滴
功能：在原始 GIF 的每一帧上绘制泪滴，然后转换回 C 数组
作者：AI Assistant
日期：2025-10-17
"""

import sys
from pathlib import Path

def main():
    try:
        from PIL import Image, ImageDraw
        print("✅ PIL/Pillow 库已安装")
    except ImportError:
        print("=" * 60)
        print("❌ 需要安装 Pillow 库")
        print("=" * 60)
        print()
        print("请运行以下命令安装：")
        print("  pip3 install Pillow")
        print()
        print("或：")
        print("  python3 -m pip install Pillow")
        print()
        return 1
    
    import re
    
    print("=" * 60)
    print("Otto 机器人 - 在 GIF 上绘制泪滴动画")
    print("=" * 60)
    print()
    
    script_dir = Path(__file__).parent
    component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    
    # 使用黄色备份作为基础
    backup_file = component_dir / 'sad.c.yellow_backup'
    
    if not backup_file.exists():
        print(f"❌ 找不到备份文件: {backup_file}")
        return 1
    
    print(f"步骤 1: 从 C 文件提取 GIF 数据...")
    with open(backup_file, 'r') as f:
        content = f.read()
    
    hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
    matches = hex_pattern.findall(content)
    gif_bytes = bytes([int(h, 16) for h in matches])
    
    print(f"  ✓ 提取了 {len(gif_bytes)} 字节")
    
    # 保存为临时 GIF 文件
    temp_gif = script_dir / 'sad_temp.gif'
    with open(temp_gif, 'wb') as f:
        f.write(gif_bytes)
    print(f"  ✓ 保存临时文件: {temp_gif}")
    
    print(f"\n步骤 2: 打开 GIF 并分析...")
    img = Image.open(temp_gif)
    
    frames = []
    try:
        frame_count = 0
        while True:
            frames.append(img.copy())
            frame_count += 1
            img.seek(img.tell() + 1)
    except EOFError:
        pass
    
    print(f"  ✓ 找到 {len(frames)} 帧")
    print(f"  ✓ 尺寸: {frames[0].size}")
    print(f"  ✓ 模式: {frames[0].mode}")
    
    print(f"\n步骤 3: 在每一帧上绘制泪滴...")
    
    # 定义泪滴位置和形状（基于 240x240 的尺寸）
    width, height = frames[0].size
    
    # 假设眼睛在中心区域，泪滴从眼睛下方流下
    # 泪滴路径：从眼睛 (y=height/2) 流到底部
    
    tear_drops = [
        # 左边泪滴
        {'start': (int(width * 0.35), int(height * 0.55)), 'points': 8},
        # 右边泪滴
        {'start': (int(width * 0.65), int(height * 0.55)), 'points': 8},
    ]
    
    modified_frames = []
    
    for frame_idx, frame in enumerate(frames):
        # 转换为 RGB 模式以便绘制
        if frame.mode != 'RGB':
            frame = frame.convert('RGB')
        
        draw = ImageDraw.Draw(frame)
        
        # 为每一帧绘制不同长度的泪滴（动画效果）
        tear_length = (frame_idx % 10) + 5  # 5-14 像素
        
        for tear in tear_drops:
            x_start, y_start = tear['start']
            
            # 绘制泪滴（从眼睛下方向下）
            for i in range(tear['points']):
                y = y_start + i * (tear_length // tear['points'])
                
                # 泪滴形状：顶部宽，底部窄
                width_at_point = max(1, 6 - i // 2)
                
                # 绘制泪滴（亮青色 #00FFFF）
                for dx in range(-width_at_point, width_at_point + 1):
                    draw.point((x_start + dx, y), fill=(0, 255, 255))  # 亮青色
        
        # 转换回调色板模式
        frame_p = frame.convert('P', palette=Image.ADAPTIVE, colors=256)
        modified_frames.append(frame_p)
        
        print(f"  ✓ 帧 {frame_idx + 1}/{len(frames)} - 泪滴长度: {tear_length} 像素")
    
    print(f"\n步骤 4: 保存带泪滴的 GIF...")
    output_gif = script_dir / 'sad_with_tears.gif'
    
    # 保存为动画 GIF
    modified_frames[0].save(
        output_gif,
        save_all=True,
        append_images=modified_frames[1:],
        duration=100,  # 每帧 100ms
        loop=0,
        optimize=False
    )
    
    print(f"  ✓ 已保存: {output_gif}")
    
    print(f"\n步骤 5: 转换为 C 数组...")
    
    # 读取新的 GIF
    with open(output_gif, 'rb') as f:
        new_gif_bytes = f.read()
    
    print(f"  ✓ 新 GIF 大小: {len(new_gif_bytes)} 字节")
    
    # 生成 C 数组
    output_c = component_dir / 'sad.c'
    
    # 读取原始 C 文件的结构
    with open(backup_file, 'r') as f:
        original_content = f.read()
    
    # 找到数组数据的位置
    map_marker = '_map[]'
    start_idx = original_content.find(map_marker)
    brace_start = original_content.find('{', start_idx)
    
    # 找到数组结束
    brace_count = 0
    brace_end = -1
    for i in range(brace_start, len(original_content)):
        if original_content[i] == '{':
            brace_count += 1
        elif original_content[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                brace_end = i
                break
    
    # 生成新的十六进制数据
    hex_lines = []
    for i in range(0, len(new_gif_bytes), 13):
        chunk = new_gif_bytes[i:i+13]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append(f'    {hex_str},')
    
    if hex_lines:
        hex_lines[-1] = hex_lines[-1].rstrip(',')
    
    # 组合新的 C 文件
    new_c_content = (
        original_content[:brace_start+1] + '\n' +
        '\n'.join(hex_lines) + '\n' +
        original_content[brace_end:]
    )
    
    # 保存
    with open(output_c, 'w') as f:
        f.write(new_c_content)
    
    print(f"  ✓ 已保存: {output_c}")
    
    # 清理临时文件
    temp_gif.unlink()
    print(f"  ✓ 清理临时文件")
    
    print()
    print("=" * 60)
    print("✅ 完成！泪滴已添加到 sad 表情")
    print("=" * 60)
    print()
    print("💧 泪滴设计：")
    print("  • 位置：眼睛下方 (35% 和 65% 位置)")
    print("  • 颜色：亮青色 #00FFFF")
    print("  • 形状：顶部宽，底部窄（泪滴形）")
    print("  • 动画：泪滴长度在 5-14 像素间变化")
    print()
    print("📝 下一步：")
    print("  1. rm -rf build")
    print("  2. idf.py build")
    print("  3. idf.py flash")
    print()
    print("🎬 预览 GIF:")
    print(f"  {output_gif}")
    print()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())


