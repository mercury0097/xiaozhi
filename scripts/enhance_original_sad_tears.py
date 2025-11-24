#!/usr/bin/env python3
"""
Otto 机器人 - 在原有 sad 表情基础上添加蓝色和流泪效果
作者: AI Assistant
日期: 2025-10-20

功能：
1. 保留原有的眼睛动画和风格
2. 将颜色从黄色改为蓝色
3. 在现有帧上叠加绘制水滴形泪滴
"""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
    print("✅ PIL/Pillow 库已安装")
except ImportError:
    print("❌ 需要安装 Pillow 库")
    print("  运行: pip3 install Pillow")
    sys.exit(1)

import re

def extract_gif_from_c(c_file_path):
    """从 C 文件提取 GIF 数据"""
    with open(c_file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
    matches = hex_pattern.findall(content)
    gif_bytes = bytes([int(h, 16) for h in matches])
    
    return gif_bytes, content

def draw_teardrop(draw, x, y, size, color_base, color_highlight):
    """
    绘制水滴形泪滴（上圆下尖，带高光）
    """
    width = int(size * 0.6)
    height = int(size)
    
    # 上半部分：椭圆
    ellipse_bbox = [
        x - width // 2,
        y,
        x + width // 2,
        y + int(height * 0.7)
    ]
    draw.ellipse(ellipse_bbox, fill=color_base)
    
    # 下半部分：三角形尖角
    triangle_points = [
        (x, y + height),
        (x - width // 2, y + int(height * 0.6)),
        (x + width // 2, y + int(height * 0.6))
    ]
    draw.polygon(triangle_points, fill=color_base)
    
    # 高光
    highlight_bbox = [
        x - width // 4,
        y + int(height * 0.15),
        x + width // 6,
        y + int(height * 0.35)
    ]
    draw.ellipse(highlight_bbox, fill=color_highlight)

def rgb_to_hsv(r, g, b):
    """RGB 转 HSV"""
    r, g, b = r / 255.0, g / 255.0, b / 255.0
    mx = max(r, g, b)
    mn = min(r, g, b)
    df = mx - mn
    
    if mx == mn:
        h = 0
    elif mx == r:
        h = (60 * ((g - b) / df) + 360) % 360
    elif mx == g:
        h = (60 * ((b - r) / df) + 120) % 360
    elif mx == b:
        h = (60 * ((r - g) / df) + 240) % 360
    
    s = 0 if mx == 0 else df / mx
    v = mx
    
    return h, s, v

def hsv_to_rgb(h, s, v):
    """HSV 转 RGB"""
    c = v * s
    x = c * (1 - abs((h / 60) % 2 - 1))
    m = v - c
    
    if 0 <= h < 60:
        r, g, b = c, x, 0
    elif 60 <= h < 120:
        r, g, b = x, c, 0
    elif 120 <= h < 180:
        r, g, b = 0, c, x
    elif 180 <= h < 240:
        r, g, b = 0, x, c
    elif 240 <= h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    
    return int((r + m) * 255), int((g + m) * 255), int((b + m) * 255)

def yellow_to_blue(r, g, b):
    """将黄色系转换为蓝色系，保持亮度和饱和度"""
    # 纯黑保持不变
    if r == 0 and g == 0 and b == 0:
        return (r, g, b)
    
    # 检测是否为黄色系（r>50, g>40, b<100）
    is_yellow = (r > 50 and g > 40 and b < 100)
    
    # 检测是否为白色/灰色系（需要保持为蓝色高光）
    is_white_gray = (abs(r - g) < 30 and abs(r - b) < 30 and r > 100)
    
    if is_yellow or is_white_gray:
        # 转换为 HSV
        h, s, v = rgb_to_hsv(r, g, b)
        
        # 将色相改为蓝色（210度）
        h = 210
        
        # 保持饱和度和亮度
        return hsv_to_rgb(h, s, v)
    
    return (r, g, b)

def enhance_original_sad_gif(original_gif_path, output_gif_path):
    """
    在原有 GIF 基础上：
    1. 将黄色改为蓝色
    2. 叠加绘制水滴形泪滴
    """
    print(f"\n🎨 处理原始 sad 表情...")
    
    # 打开原始 GIF
    original_img = Image.open(original_gif_path)
    
    # 提取所有帧
    frames = []
    frame_count = 0
    try:
        while True:
            frames.append(original_img.copy())
            frame_count += 1
            original_img.seek(original_img.tell() + 1)
    except EOFError:
        pass
    
    print(f"  ✓ 原始帧数: {len(frames)}")
    print(f"  ✓ 尺寸: {frames[0].size}")
    
    # 处理每一帧
    enhanced_frames = []
    width, height = frames[0].size
    
    # 泪滴参数
    tear_size = 18
    tear_start_y = int(height * 0.55)
    tear_end_y = height - 20
    eye_left_x = int(width * 0.35)
    eye_right_x = int(width * 0.65)
    
    TEAR_BASE = (100, 180, 255)
    TEAR_HIGHLIGHT = (200, 230, 255)
    
    for frame_idx, frame in enumerate(frames):
        # 转换为 RGB 模式
        if frame.mode == 'P':
            # 获取调色板
            palette = frame.getpalette()
            if palette:
                # 修改调色板：黄色 → 蓝色
                new_palette = []
                for i in range(0, len(palette), 3):
                    r, g, b = palette[i], palette[i+1], palette[i+2]
                    new_r, new_g, new_b = yellow_to_blue(r, g, b)
                    new_palette.extend([new_r, new_g, new_b])
                
                frame.putpalette(new_palette)
        
        # 转换为 RGB 用于绘制
        frame_rgb = frame.convert('RGB')
        draw = ImageDraw.Draw(frame_rgb)
        
        # 计算泪滴位置（循环流动）
        # 左侧泪滴
        left_cycle = len(frames) * 2
        left_phase = (frame_idx * 2) % left_cycle
        if left_phase < len(frames):
            left_progress = left_phase / len(frames)
            left_tear_y = tear_start_y + int((tear_end_y - tear_start_y) * left_progress)
            if left_tear_y < tear_end_y:
                draw_teardrop(
                    draw,
                    eye_left_x,
                    left_tear_y,
                    tear_size,
                    TEAR_BASE,
                    TEAR_HIGHLIGHT
                )
        
        # 右侧泪滴（错峰）
        right_phase = (frame_idx * 2 + len(frames) // 3) % left_cycle
        if right_phase < len(frames):
            right_progress = right_phase / len(frames)
            right_tear_y = tear_start_y + int((tear_end_y - tear_start_y) * right_progress)
            if right_tear_y < tear_end_y:
                draw_teardrop(
                    draw,
                    eye_right_x,
                    right_tear_y,
                    tear_size,
                    TEAR_BASE,
                    TEAR_HIGHLIGHT
                )
        
        # 转换回调色板模式
        frame_p = frame_rgb.convert('P', palette=Image.ADAPTIVE, colors=256)
        enhanced_frames.append(frame_p)
        
        if (frame_idx + 1) % 10 == 0:
            print(f"  ✓ 已处理 {frame_idx + 1}/{len(frames)} 帧")
    
    # 保存为 GIF
    enhanced_frames[0].save(
        output_gif_path,
        save_all=True,
        append_images=enhanced_frames[1:],
        duration=100,
        loop=0,
        optimize=False
    )
    
    print(f"\n✅ 已保存: {output_gif_path}")
    print(f"  文件大小: {output_gif_path.stat().st_size} 字节")

def convert_gif_to_c_array(gif_path, output_c_path, backup_c_path):
    """将 GIF 转换为 C 数组"""
    print(f"\n📝 转换为 C 数组...")
    
    # 备份
    if not backup_c_path.exists():
        import shutil
        shutil.copy2(output_c_path, backup_c_path)
        print(f"  ✓ 已备份: {backup_c_path.name}")
    else:
        print(f"  ℹ️  备份已存在: {backup_c_path.name}")
    
    # 读取新 GIF
    with open(gif_path, 'rb') as f:
        new_gif_bytes = f.read()
    
    print(f"  新 GIF 大小: {len(new_gif_bytes)} 字节")
    
    # 读取原始 C 文件结构
    with open(backup_c_path, 'r') as f:
        original_content = f.read()
    
    # 找到数组数据位置
    map_marker = '_map[]'
    start_idx = original_content.find(map_marker)
    brace_start = original_content.find('{', start_idx)
    
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
    
    # 生成十六进制数据
    hex_lines = []
    for i in range(0, len(new_gif_bytes), 13):
        chunk = new_gif_bytes[i:i+13]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append(f'    {hex_str},')
    
    if hex_lines:
        hex_lines[-1] = hex_lines[-1].rstrip(',')
    
    # 组合新 C 文件
    new_c_content = (
        original_content[:brace_start+1] + '\n' +
        '\n'.join(hex_lines) + '\n' +
        original_content[brace_end:]
    )
    
    # 保存
    with open(output_c_path, 'w') as f:
        f.write(new_c_content)
    
    print(f"  ✓ 已更新: {output_c_path}")

def main():
    script_dir = Path(__file__).parent
    component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    
    sad_c_file = component_dir / 'sad.c'
    backup_file = component_dir / 'sad.c.original_backup'
    temp_gif = script_dir / 'sad_temp.gif'
    output_gif = script_dir / 'sad_enhanced.gif'
    
    print("=" * 70)
    print("🎭 在原有 sad 表情基础上添加蓝色和流泪效果")
    print("=" * 70)
    print()
    print("📋 处理方式:")
    print("  ✓ 保留原有的眼睛动画和表情")
    print("  ✓ 将颜色从黄色改为蓝色")
    print("  ✓ 叠加绘制水滴形泪滴")
    print()
    
    if not sad_c_file.exists():
        print(f"❌ 错误：找不到 {sad_c_file}")
        return 1
    
    try:
        # 提取原始 GIF
        print("步骤 1: 提取原始 GIF...")
        gif_bytes, _ = extract_gif_from_c(sad_c_file)
        with open(temp_gif, 'wb') as f:
            f.write(gif_bytes)
        print(f"  ✓ 已提取: {temp_gif}")
        
        # 处理 GIF
        print("\n步骤 2: 处理 GIF（改色 + 添加泪滴）...")
        enhance_original_sad_gif(temp_gif, output_gif)
        
        # 转换为 C 数组
        print("\n步骤 3: 转换为 C 数组...")
        convert_gif_to_c_array(output_gif, sad_c_file, backup_file)
        
        # 清理临时文件
        temp_gif.unlink()
        print(f"\n  ✓ 已清理临时文件")
        
        print()
        print("=" * 70)
        print("✅ 完成！已在原有表情基础上添加蓝色和流泪效果")
        print("=" * 70)
        print()
        print("🎨 改进内容:")
        print("  • 保留了原有的眼睛动画（眨眼、表情变化等）")
        print("  • 将眼睛颜色从黄色改为蓝色")
        print("  • 添加了水滴形流泪效果")
        print("  • 泪滴左右错峰流动")
        print()
        print("📁 文件位置:")
        print(f"  • GIF 预览: {output_gif}")
        print(f"  • 更新文件: {sad_c_file}")
        print(f"  • 备份文件: {backup_file}")
        print()
        print("🚀 下一步:")
        print("  1. 预览 GIF 确认效果")
        print("  2. rm -rf build")
        print("  3. idf.py build")
        print("  4. idf.py flash")
        print()
        
        return 0
        
    except Exception as e:
        print(f"\n❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


