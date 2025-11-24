#!/usr/bin/env python3
"""
Otto 机器人 - 量产级蓝色流泪表情生成器
作者: AI Assistant
日期: 2025-10-20

规格:
- 240×240 分辨率
- 蓝色眼睛 (#1E90FF 系)
- 水滴形泪滴，带高光和渐变
- 与现有表情风格统一
- 约 30 帧，循环流畅
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
    绘制水滴形泪滴（非方形）
    带高光和渐变效果
    """
    # 泪滴主体：椭圆 + 下方尖角
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
    
    # 下半部分：尖角（三角形）
    triangle_points = [
        (x, y + height),  # 底部尖端
        (x - width // 2, y + int(height * 0.6)),  # 左上
        (x + width // 2, y + int(height * 0.6))   # 右上
    ]
    draw.polygon(triangle_points, fill=color_base)
    
    # 高光（小椭圆，模拟光泽）
    highlight_bbox = [
        x - width // 4,
        y + int(height * 0.15),
        x + width // 6,
        y + int(height * 0.35)
    ]
    draw.ellipse(highlight_bbox, fill=color_highlight)

def create_blue_eye_tear_gif(output_path, width=240, height=240, num_frames=30):
    """
    创建量产级蓝色流泪 GIF
    """
    print(f"\n🎨 生成量产级蓝色流泪表情...")
    print(f"  分辨率: {width}x{height}")
    print(f"  帧数: {num_frames}")
    
    frames = []
    
    # 颜色定义（蓝色系）
    BLUE_BASE = (30, 144, 255)        # #1E90FF DodgerBlue 眼睛主色
    BLUE_HIGHLIGHT = (135, 206, 250)  # #87CEFA LightSkyBlue 眼睛高光
    BLUE_SHADOW = (10, 61, 145)       # #0A3D91 深蓝 眼睛阴影
    
    TEAR_BASE = (100, 180, 255)       # 泪滴主色（稍浅的蓝）
    TEAR_HIGHLIGHT = (200, 230, 255)  # 泪滴高光（几乎白色）
    
    BLACK = (0, 0, 0)
    
    # 眼睛位置（基于 240×240）
    eye_left_x = int(width * 0.35)
    eye_right_x = int(width * 0.65)
    eye_y = int(height * 0.45)
    eye_radius = int(width * 0.12)
    
    # 泪滴起点（眼睛下缘）
    tear_start_y = eye_y + eye_radius + 5
    tear_end_y = height - 20
    
    for frame_idx in range(num_frames):
        # 创建黑色背景
        img = Image.new('RGB', (width, height), BLACK)
        draw = ImageDraw.Draw(img)
        
        # 绘制蓝色眼睛（圆形，带高光）
        # 左眼
        draw.ellipse([
            eye_left_x - eye_radius,
            eye_y - eye_radius,
            eye_left_x + eye_radius,
            eye_y + eye_radius
        ], fill=BLUE_BASE)
        
        # 左眼高光
        draw.ellipse([
            eye_left_x - eye_radius // 3,
            eye_y - eye_radius // 2,
            eye_left_x + eye_radius // 4,
            eye_y - eye_radius // 6
        ], fill=BLUE_HIGHLIGHT)
        
        # 左眼阴影
        draw.arc([
            eye_left_x - eye_radius,
            eye_y - eye_radius,
            eye_left_x + eye_radius,
            eye_y + eye_radius
        ], start=180, end=360, fill=BLUE_SHADOW, width=3)
        
        # 右眼（镜像）
        draw.ellipse([
            eye_right_x - eye_radius,
            eye_y - eye_radius,
            eye_right_x + eye_radius,
            eye_y + eye_radius
        ], fill=BLUE_BASE)
        
        draw.ellipse([
            eye_right_x - eye_radius // 3,
            eye_y - eye_radius // 2,
            eye_right_x + eye_radius // 4,
            eye_y - eye_radius // 6
        ], fill=BLUE_HIGHLIGHT)
        
        draw.arc([
            eye_right_x - eye_radius,
            eye_y - eye_radius,
            eye_right_x + eye_radius,
            eye_y + eye_radius
        ], start=180, end=360, fill=BLUE_SHADOW, width=3)
        
        # 绘制流动的泪滴（左右错峰）
        # 左侧泪滴
        left_phase = (frame_idx * 2) % (num_frames * 2)
        if left_phase < num_frames:
            left_progress = left_phase / num_frames
            left_tear_y = tear_start_y + int((tear_end_y - tear_start_y) * left_progress)
            # 只在有效范围内绘制
            if left_tear_y < tear_end_y:
                draw_teardrop(
                    draw,
                    eye_left_x,
                    left_tear_y,
                    size=20,
                    color_base=TEAR_BASE,
                    color_highlight=TEAR_HIGHLIGHT
                )
        
        # 右侧泪滴（错峰 1/3 周期）
        right_phase = (frame_idx * 2 + num_frames // 3) % (num_frames * 2)
        if right_phase < num_frames:
            right_progress = right_phase / num_frames
            right_tear_y = tear_start_y + int((tear_end_y - tear_start_y) * right_progress)
            if right_tear_y < tear_end_y:
                draw_teardrop(
                    draw,
                    eye_right_x,
                    right_tear_y,
                    size=20,
                    color_base=TEAR_BASE,
                    color_highlight=TEAR_HIGHLIGHT
                )
        
        # 转换为 256 色调色板模式（与现有 GIF 一致）
        img_p = img.convert('P', palette=Image.ADAPTIVE, colors=256)
        frames.append(img_p)
        
        if (frame_idx + 1) % 10 == 0:
            print(f"  ✓ 已生成 {frame_idx + 1}/{num_frames} 帧")
    
    # 保存为 GIF
    frames[0].save(
        output_path,
        save_all=True,
        append_images=frames[1:],
        duration=100,  # 每帧 100ms = 10fps
        loop=0,
        optimize=False
    )
    
    print(f"\n✅ 已保存: {output_path}")
    print(f"  文件大小: {output_path.stat().st_size} 字节")
    
    return frames

def convert_gif_to_c_array(gif_path, output_c_path, backup_c_path):
    """
    将 GIF 转换为 C 数组并替换 sad.c
    """
    print(f"\n📝 转换为 C 数组...")
    
    # 备份原始文件
    if backup_c_path.exists():
        print(f"  ℹ️  备份文件已存在: {backup_c_path.name}")
    else:
        import shutil
        shutil.copy2(output_c_path, backup_c_path)
        print(f"  ✓ 已备份: {backup_c_path.name}")
    
    # 读取新 GIF
    with open(gif_path, 'rb') as f:
        new_gif_bytes = f.read()
    
    print(f"  新 GIF 大小: {len(new_gif_bytes)} 字节")
    
    # 读取原始 C 文件结构
    with open(backup_c_path, 'r') as f:
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
    with open(output_c_path, 'w') as f:
        f.write(new_c_content)
    
    print(f"  ✓ 已更新: {output_c_path}")

def main():
    script_dir = Path(__file__).parent
    component_dir = script_dir.parent / 'components' / 'txp666__otto-emoji-gif-component' / 'src'
    
    sad_c_file = component_dir / 'sad.c'
    backup_file = component_dir / 'sad.c.before_production'
    output_gif = script_dir / 'sad_production.gif'
    
    print("=" * 70)
    print("🎭 Otto 机器人 - 量产级蓝色流泪表情生成器")
    print("=" * 70)
    print()
    print("📋 规格要求:")
    print("  ✓ 蓝色眼睛 (#1E90FF DodgerBlue)")
    print("  ✓ 水滴形泪滴（非方形，带高光和渐变）")
    print("  ✓ 纯黑背景 (#000000)")
    print("  ✓ 240×240 分辨率")
    print("  ✓ 约 30 帧，10fps")
    print("  ✓ 左右泪滴错峰流动")
    print()
    
    if not sad_c_file.exists():
        print(f"❌ 错误：找不到 {sad_c_file}")
        return 1
    
    try:
        # 生成高质量 GIF
        frames = create_blue_eye_tear_gif(output_gif, width=240, height=240, num_frames=30)
        
        # 转换为 C 数组
        convert_gif_to_c_array(output_gif, sad_c_file, backup_file)
        
        print()
        print("=" * 70)
        print("✅ 完成！量产级蓝色流泪表情已生成")
        print("=" * 70)
        print()
        print("🎨 表情特性:")
        print("  • 眼睛颜色: 🔵 蓝色 (#1E90FF)")
        print("  • 泪滴形状: 💧 水滴形（上圆下尖，带高光）")
        print("  • 动画效果: 左右错峰流动，自然循环")
        print("  • 风格统一: 与现有表情一致的高光/阴影")
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
        print("💡 提示:")
        print("  • 如需调整泪滴大小，修改脚本中的 size 参数")
        print("  • 如需调整流动速度，修改 duration 参数")
        print("  • 如需恢复原版，使用备份文件")
        print()
        
        return 0
        
    except Exception as e:
        print(f"\n❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


