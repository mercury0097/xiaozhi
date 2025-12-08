/**
 * @file eye_drawer.cc
 * @brief LVGL 眼睛绘制器实现 (LVGL 9 兼容)
 */

#include "eye_drawer.h"
#include <algorithm>
#include <cmath>

namespace vector_eyes {

// 静态成员初始化
lv_obj_t* EyeDrawer::canvas_ = nullptr;
lv_color_t EyeDrawer::draw_color_ = lv_color_white();
lv_color_t EyeDrawer::bg_color_ = lv_color_black();

void EyeDrawer::SetCanvas(lv_obj_t* canvas) {
    canvas_ = canvas;
}

void EyeDrawer::SetColor(lv_color_t color) {
    draw_color_ = color;
}

void EyeDrawer::Clear(lv_color_t bg_color) {
    bg_color_ = bg_color;
    if (canvas_) {
        lv_canvas_fill_bg(canvas_, bg_color, LV_OPA_COVER);
    }
}

// 使用 lv_canvas_set_px 逐像素绘制矩形
void EyeDrawer::FillRectangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool fill) {
    if (!canvas_) return;
    
    int32_t l = std::min(x0, x1);
    int32_t r = std::max(x0, x1);
    int32_t t = std::min(y0, y1);
    int32_t b = std::max(y0, y1);
    
    if (r <= l || b <= t) return;

    lv_color_t color = fill ? draw_color_ : bg_color_;
    
    for (int32_t y = t; y < b; y++) {
        for (int32_t x = l; x < r; x++) {
            lv_canvas_set_px(canvas_, x, y, color, LV_OPA_COVER);
        }
    }
}

void EyeDrawer::FillRectangularTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool fill) {
    if (!canvas_) return;
    if (x0 == x1 || y0 == y1) return;

    lv_color_t color = fill ? draw_color_ : bg_color_;

    // 简化：直角三角形扫描线填充
    int32_t min_y = std::min(y0, y1);
    int32_t max_y = std::max(y0, y1);
    
    float slope = static_cast<float>(x1 - x0) / (y1 - y0);

    for (int32_t y = min_y; y <= max_y; y++) {
        float x_edge = x0 + (y - y0) * slope;
        int32_t x_start = std::min(static_cast<int32_t>(x_edge), x1);
        int32_t x_end = std::max(static_cast<int32_t>(x_edge), x1);
        
        for (int32_t x = x_start; x <= x_end; x++) {
            lv_canvas_set_px(canvas_, x, y, color, LV_OPA_COVER);
        }
    }
}

void EyeDrawer::FillEllipseCorner(CornerType corner, int16_t x0, int16_t y0, 
                                   int32_t rx, int32_t ry, bool fill) {
    if (!canvas_ || rx < 2 || ry < 2) return;

    lv_color_t color = fill ? draw_color_ : bg_color_;

    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t fx2 = 4 * rx2;
    int32_t fy2 = 4 * ry2;

    auto drawHLine = [&](int32_t x_start, int32_t y_pos, int32_t width) {
        for (int32_t i = 0; i < width; i++) {
            lv_canvas_set_px(canvas_, x_start + i, y_pos, color, LV_OPA_COVER);
        }
    };

    int32_t x, y, s;

    switch (corner) {
        case CornerType::TopRight:
            for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
                drawHLine(x0, y0 - y, x);
                if (s >= 0) { s += fx2 * (1 - y); y--; }
                s += ry2 * ((4 * x) + 6);
            }
            for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
                drawHLine(x0, y0 - y, x);
                if (s >= 0) { s += fy2 * (1 - x); x--; }
                s += rx2 * ((4 * y) + 6);
            }
            break;

        case CornerType::BottomRight:
            for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
                drawHLine(x0, y0 + y - 1, x);
                if (s >= 0) { s += fx2 * (1 - y); y--; }
                s += ry2 * ((4 * x) + 6);
            }
            for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
                drawHLine(x0, y0 + y - 1, x);
                if (s >= 0) { s += fy2 * (1 - x); x--; }
                s += rx2 * ((4 * y) + 6);
            }
            break;

        case CornerType::TopLeft:
            for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
                drawHLine(x0 - x, y0 - y, x);
                if (s >= 0) { s += fx2 * (1 - y); y--; }
                s += ry2 * ((4 * x) + 6);
            }
            for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
                drawHLine(x0 - x, y0 - y, x);
                if (s >= 0) { s += fy2 * (1 - x); x--; }
                s += rx2 * ((4 * y) + 6);
            }
            break;

        case CornerType::BottomLeft:
            for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
                drawHLine(x0 - x, y0 + y - 1, x);
                if (s >= 0) { s += fx2 * (1 - y); y--; }
                s += ry2 * ((4 * x) + 6);
            }
            for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
                drawHLine(x0 - x, y0 + y, x);
                if (s >= 0) { s += fy2 * (1 - x); x--; }
                s += rx2 * ((4 * y) + 6);
            }
            break;
    }
}

void EyeDrawer::Draw(int16_t center_x, int16_t center_y, EyeConfig* config) {
    if (!canvas_ || !config) return;

    // 计算斜度导致的 Y 偏移
    int32_t delta_y_top = config->height * config->slope_top / 2.0f;
    int32_t delta_y_bottom = config->height * config->slope_bottom / 2.0f;

    // 计算总高度
    auto total_height = config->height + delta_y_top - delta_y_bottom;

    // 如果圆角超过高度，调整圆角大小
    int16_t radius_top = config->radius_top;
    int16_t radius_bottom = config->radius_bottom;
    
    if (radius_bottom > 0 && radius_top > 0 && 
        total_height - 1 < radius_bottom + radius_top) {
        float ratio = static_cast<float>(total_height - 1) / (radius_bottom + radius_top);
        radius_top = static_cast<int16_t>(radius_top * ratio);
        radius_bottom = static_cast<int16_t>(radius_bottom * ratio);
    }

    // 计算四个角的内侧坐标
    int32_t TLc_y = center_y + config->offset_y - config->height / 2 + radius_top - delta_y_top;
    int32_t TLc_x = center_x + config->offset_x - config->width / 2 + radius_top;
    int32_t TRc_y = center_y + config->offset_y - config->height / 2 + radius_top + delta_y_top;
    int32_t TRc_x = center_x + config->offset_x + config->width / 2 - radius_top;
    int32_t BLc_y = center_y + config->offset_y + config->height / 2 - radius_bottom - delta_y_bottom;
    int32_t BLc_x = center_x + config->offset_x - config->width / 2 + radius_bottom;
    int32_t BRc_y = center_y + config->offset_y + config->height / 2 - radius_bottom + delta_y_bottom;
    int32_t BRc_x = center_x + config->offset_x + config->width / 2 - radius_bottom;

    // 计算内部范围
    int32_t min_c_x = std::min(TLc_x, BLc_x);
    int32_t max_c_x = std::max(TRc_x, BRc_x);
    int32_t min_c_y = std::min(TLc_y, TRc_y);
    int32_t max_c_y = std::max(BLc_y, BRc_y);

    // 填充眼睛中心
    FillRectangle(min_c_x, min_c_y, max_c_x, max_c_y, true);

    // 填充到圆角边缘
    FillRectangle(TRc_x, TRc_y, BRc_x + radius_bottom, BRc_y, true);  // 右
    FillRectangle(TLc_x - radius_top, TLc_y, BLc_x, BLc_y, true);     // 左
    FillRectangle(TLc_x, TLc_y - radius_top, TRc_x, TRc_y, true);     // 上
    FillRectangle(BLc_x, BLc_y, BRc_x, BRc_y + radius_bottom, true);  // 下

    // 绘制斜边（上）
    if (config->slope_top > 0) {
        FillRectangularTriangle(TLc_x, TLc_y - radius_top, TRc_x, TRc_y - radius_top, false);
        FillRectangularTriangle(TRc_x, TRc_y - radius_top, TLc_x, TLc_y - radius_top, true);
    } else if (config->slope_top < 0) {
        FillRectangularTriangle(TRc_x, TRc_y - radius_top, TLc_x, TLc_y - radius_top, false);
        FillRectangularTriangle(TLc_x, TLc_y - radius_top, TRc_x, TRc_y - radius_top, true);
    }

    // 绘制斜边（下）
    if (config->slope_bottom > 0) {
        FillRectangularTriangle(BRc_x + radius_bottom, BRc_y + radius_bottom, 
                                BLc_x - radius_bottom, BLc_y + radius_bottom, false);
        FillRectangularTriangle(BLc_x - radius_bottom, BLc_y + radius_bottom, 
                                BRc_x + radius_bottom, BRc_y + radius_bottom, true);
    } else if (config->slope_bottom < 0) {
        FillRectangularTriangle(BLc_x - radius_bottom, BLc_y + radius_bottom, 
                                BRc_x + radius_bottom, BRc_y + radius_bottom, false);
        FillRectangularTriangle(BRc_x + radius_bottom, BRc_y + radius_bottom, 
                                BLc_x - radius_bottom, BLc_y + radius_bottom, true);
    }

    // 绘制圆角
    if (radius_top > 0) {
        FillEllipseCorner(CornerType::TopLeft, TLc_x, TLc_y, radius_top, radius_top, true);
        FillEllipseCorner(CornerType::TopRight, TRc_x, TRc_y, radius_top, radius_top, true);
    }
    if (radius_bottom > 0) {
        FillEllipseCorner(CornerType::BottomLeft, BLc_x, BLc_y, radius_bottom, radius_bottom, true);
        FillEllipseCorner(CornerType::BottomRight, BRc_x, BRc_y, radius_bottom, radius_bottom, true);
    }
}

} // namespace vector_eyes
