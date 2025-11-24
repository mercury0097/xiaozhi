#!/usr/bin/env python3
"""
Otto 机器人 - 在 anger 表情基础上添加顶部火焰特效（连续火带版）
作者: AI Assistant
日期: 2025-10-20

功能：
1. 保留原有的红色眼睛和所有动画
2. 在屏幕顶部叠加火焰特效
3. 连续“火带”效果（全宽），流动边缘 + 闪烁 + 渐变
"""

import sys
from pathlib import Path
import math
import random

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

def draw_flame(draw, x, y, width, height, intensity, colors):
    """
    绘制火焰
    x, y: 底部中心点
    width: 底部宽度
    height: 火焰高度
    intensity: 强度 0.0-1.0（控制火焰大小和亮度）
    colors: 颜色列表 [底部色, 中间色, 顶部色]
    """
    # 火焰形状：底部宽，顶部尖
    # 使用多个椭圆叠加模拟火焰效果
    
    color_bottom, color_mid, color_top = colors
    
    # 调整高度和宽度基于强度
    actual_height = int(height * intensity)
    actual_width = int(width * intensity)
    
    # 底部（最亮，黄色/橙色）
    bottom_height = int(actual_height * 0.4)
    draw.ellipse([
        x - actual_width // 2,
        y - bottom_height,
        x + actual_width // 2,
        y
    ], fill=color_bottom)
    
    # 中部（橙红色）
    mid_height = int(actual_height * 0.7)
    mid_width = int(actual_width * 0.7)
    draw.ellipse([
        x - mid_width // 2,
        y - mid_height,
        x + mid_width // 2,
        y - bottom_height // 2
    ], fill=color_mid)
    
    # 顶部（深红色，尖角）
    top_width = int(actual_width * 0.4)
    top_y = y - actual_height
    
    # 用三角形模拟火焰尖端
    points = [
        (x, top_y),  # 顶点
        (x - top_width // 2, y - mid_height),  # 左下
        (x + top_width // 2, y - mid_height)   # 右下
    ]
    draw.polygon(points, fill=color_top)
    
    # 高光（模拟火焰核心）
    highlight_y = y - int(actual_height * 0.2)
    highlight_width = int(actual_width * 0.3)
    draw.ellipse([
        x - highlight_width // 2,
        highlight_y - 3,
        x + highlight_width // 2,
        highlight_y + 3
    ], fill=(255, 255, 200))  # 亮黄色高光

def _lerp(a, b, t):
    return int(a + (b - a) * t)


def _lerp_color(c1, c2, t):
    return (
        _lerp(c1[0], c2[0], t),
        _lerp(c1[1], c2[1], t),
        _lerp(c1[2], c2[2], t),
    )


def draw_fire_effect(draw, frame_idx, total_frames, screen_width, screen_height):
    """
    绘制顶部火焰特效（连续火带，具有流动边缘与闪烁）
    """
    # 渐变颜色（自下而上）：黄 → 橙 → 红
    YELLOW = (255, 210, 0)
    ORANGE = (255, 120, 0)
    RED = (220, 30, 0)
    HIGHLIGHT = (255, 255, 200)

    # 火带参数（相对于屏幕尺寸）
    band_bottom_y = int(screen_height * 0.24)   # 火带底部位置（距离顶部约 24%）
    base_band_height = int(screen_height * 0.26)  # 火带平均高度（约 26% 屏高），更旺盛

    # 通过多频正弦 + 轻微随机，生成随帧变化的上边缘轮廓
    # 为了“连续”，我们为每个 x 计算一个 top_y，并按列填充渐变
    # 顶部轮廓最大起伏
    ripple1_amp = int(base_band_height * 0.50)
    ripple2_amp = int(base_band_height * 0.30)

    # 随帧流动速度（频率）
    t = frame_idx / max(1, total_frames)
    flow1 = 2 * math.pi * (t * 1.0)
    flow2 = 2 * math.pi * (t * 2.2)

    # 列渲染
    for x in range(screen_width):
        # 归一化横坐标
        xf = x / max(1, (screen_width - 1))

        # 两个不同频率/相位的波形叠加，形成有机边缘
        y_offset = (
            ripple1_amp * math.sin(2 * math.pi * (2.0 * xf) + flow1)
            + ripple2_amp * math.sin(2 * math.pi * (5.0 * xf + 0.3) - flow2)
        )

        # 轻微抖动，制造闪烁（固定随机种子下的伪随机）
        # 注意：不使用全局 random 来避免帧间不可控的跳跃
        jitter = (math.sin(2 * math.pi * (12.3 * xf + t * 5.1)) * 0.8)

        # 顶部 y（离屏幕顶部更近）
        top_y = int(band_bottom_y - (base_band_height * 0.6 + y_offset + jitter))
        if top_y < 0:
            top_y = 0
        if top_y > band_bottom_y - 1:
            top_y = band_bottom_y - 1

        column_height = max(1, band_bottom_y - top_y)

        # 列内从上到下绘制渐变像素
        for i in range(column_height):
            y = top_y + i

            # 归一化层内位置 (0 顶部 → 1 底部)
            v = i / (column_height - 1) if column_height > 1 else 1.0

            # 三段式渐变：
            # 顶部 0~0.4: 红 → 橙
            # 中部 0.4~0.85: 橙 → 黄
            # 底部 0.85~1.0: 黄 + 高光
            if v < 0.40:
                c = _lerp_color(RED, ORANGE, v / 0.40)
            elif v < 0.85:
                c = _lerp_color(ORANGE, YELLOW, (v - 0.40) / (0.45))
            else:
                core_t = (v - 0.85) / 0.15
                c = _lerp_color(YELLOW, HIGHLIGHT, core_t)

            # 亮度闪烁（列相关 + 帧相关），保持细微避免“频闪”感
            flicker = 0.90 + 0.12 * math.sin(2 * math.pi * (7.0 * xf + t * 3.5))
            c = (
                max(0, min(255, int(c[0] * flicker))),
                max(0, min(255, int(c[1] * flicker))),
                max(0, min(255, int(c[2] * flicker)))
            )

            draw.point((x, y), fill=c)

def add_fire_to_anger_gif(original_gif_path, output_gif_path):
    """
    在原有 anger GIF 基础上添加顶部火焰特效
    """
    print(f"\n🔥 处理 anger 表情，添加火焰特效...")
    
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
    
    # 设置随机种子，确保火焰效果可重现
    random.seed(42)
    
    for frame_idx, frame in enumerate(frames):
        # 转换为 RGB 用于绘制
        frame_rgb = frame.convert('RGB')
        draw = ImageDraw.Draw(frame_rgb)
        
        # 绘制火焰特效
        draw_fire_effect(draw, frame_idx, len(frames), width, height)
        
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
    
    anger_c_file = component_dir / 'anger.c'
    backup_file = component_dir / 'anger.c.before_fire'
    temp_gif = script_dir / 'anger_temp.gif'
    output_gif = script_dir / 'anger_with_fire.gif'
    
    print("=" * 70)
    print("🔥 在 anger 表情基础上添加顶部火焰特效")
    print("=" * 70)
    print()
    print("📋 处理方式:")
    print("  ✓ 保留原有的红色眼睛和所有动画")
    print("  ✓ 在屏幕顶部叠加火焰特效")
    print("  ✓ 连续火带：全宽，流动边缘 + 闪烁")
    print("  ✓ 颜色渐变：黄 → 橙 → 红")
    print()
    
    if not anger_c_file.exists():
        print(f"❌ 错误：找不到 {anger_c_file}")
        return 1
    
    try:
        # 提取原始 GIF
        print("步骤 1: 提取原始 GIF...")
        gif_bytes, _ = extract_gif_from_c(anger_c_file)
        with open(temp_gif, 'wb') as f:
            f.write(gif_bytes)
        print(f"  ✓ 已提取: {temp_gif}")
        
        # 处理 GIF
        print("\n步骤 2: 添加火焰特效...")
        add_fire_to_anger_gif(temp_gif, output_gif)
        
        # 转换为 C 数组
        print("\n步骤 3: 转换为 C 数组...")
        convert_gif_to_c_array(output_gif, anger_c_file, backup_file)
        
        # 清理临时文件
        temp_gif.unlink()
        print(f"\n  ✓ 已清理临时文件")
        
        print()
        print("=" * 70)
        print("✅ 完成！anger 表情已添加火焰特效")
        print("=" * 70)
        print()
        print("🔥 火焰特效:")
        print("  • 位置: 屏幕顶部（约 18% 底边）")
        print("  • 形态: 顶部连续火带（非单个火苗）")
        print("  • 颜色: 黄 → 橙 → 红渐变")
        print("  • 动画: 流动边缘 + 轻微闪烁")
        print("  • 高光: 亮黄色核心（靠近底部）")
        print()
        print("👁️ 眼睛:")
        print("  • 完全保留原有的红色眼睛")
        print("  • 所有动画和表情变化都保留")
        print()
        print("📁 文件位置:")
        print(f"  • GIF 预览: {output_gif}")
        print(f"  • 更新文件: {anger_c_file}")
        print(f"  • 备份文件: {backup_file}")
        print()
        print("🚀 下一步:")
        print("  1. 预览 GIF 确认效果")
        print("  2. rm -rf build")
        print("  3. idf.py build")
        print("  4. idf.py flash")
        print()
        print("🎭 触发方式:")
        print('  对机器人说: "假装你很生气"')
        print()
        
        return 0
        
    except Exception as e:
        print(f"\n❌ 处理时出错: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())


