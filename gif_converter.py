import sys
import os
import struct

def convert_gif_to_c(gif_path):
    if not os.path.exists(gif_path):
        print(f"Error: File {gif_path} not found.")
        return

    filename = os.path.basename(gif_path)
    name_no_ext = os.path.splitext(filename)[0]
    # Ensure variable name is valid C identifier
    var_name = "".join(c if c.isalnum() else "_" for c in name_no_ext)
    
    with open(gif_path, 'rb') as f:
        data = f.read()

    size = len(data)
    
    # Parse GIF header for width and height
    # Header: Signature (3) + Version (3) + Width (2) + Height (2)
    if size < 10:
        print("Error: File is too small to be a valid GIF.")
        return
        
    width = data[6] | (data[7] << 8)
    height = data[8] | (data[9] << 8)
    
    print(f"Processing {filename}: Size={size} bytes, Resolution={width}x{height}")

    c_content = f"""#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_{var_name.upper()}
#define LV_ATTRIBUTE_IMG_{var_name.upper()}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{var_name.upper()} uint8_t {var_name}_map[] = {{
"""

    # Add data bytes
    for i in range(0, size, 16):
        chunk = data[i:i+16]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        c_content += f"    {hex_str},\n"

    c_content += "};\n\n"

    c_content += f"""const lv_img_dsc_t {var_name} = {{
  .header.cf = LV_COLOR_FORMAT_RAW,
  .header.w = {width},
  .header.h = {height},
  .data_size = {size},
  .data = {var_name}_map,
}};
"""

    output_filename = f"{name_no_ext}.c"
    with open(output_filename, 'w') as f:
        f.write(c_content)
    
    print(f"Successfully created {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 gif_converter.py <path_to_gif_file>")
        print("Example: python3 gif_converter.py happy.gif")
    else:
        for gif_file in sys.argv[1:]:
            convert_gif_to_c(gif_file)
