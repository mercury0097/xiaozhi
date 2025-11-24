#!/usr/bin/env python3
"""检查 model 分区配置和状态"""

import os
import sys

def check_partition_table():
    """检查 partition table 配置"""
    print("=" * 60)
    print("🔍 检查 Partition Table 配置")
    print("=" * 60)
    
    partition_file = "partitions/v2/16m.csv"
    if os.path.exists(partition_file):
        print(f"✅ Partition table 文件存在: {partition_file}")
        with open(partition_file, 'r') as f:
            for line in f:
                if 'model' in line.lower():
                    print(f"   找到 model 分区: {line.strip()}")
    else:
        print(f"❌ Partition table 文件不存在: {partition_file}")
    
    print()

def check_srmodels_bin():
    """检查 srmodels.bin 文件"""
    print("=" * 60)
    print("🔍 检查 srmodels.bin 文件")
    print("=" * 60)
    
    srmodels_path = "build/srmodels/srmodels.bin"
    if os.path.exists(srmodels_path):
        size = os.path.getsize(srmodels_path)
        print(f"✅ srmodels.bin 存在")
        print(f"   路径: {srmodels_path}")
        print(f"   大小: {size / 1024:.1f} KB")
        
        # 检查是否包含 VADNet1
        with open(srmodels_path, 'rb') as f:
            content = f.read()
            if b'vadnet1' in content:
                print(f"   ✅ 包含 vadnet1 字符串")
            if b'vadn1' in content:
                print(f"   ✅ 包含 vadn1 字符串")
    else:
        print(f"❌ srmodels.bin 不存在: {srmodels_path}")
    
    print()

def check_flash_args():
    """检查 flash 参数"""
    print("=" * 60)
    print("🔍 检查 Flash 参数")
    print("=" * 60)
    
    flash_args_file = "build/flash_project_args"
    if os.path.exists(flash_args_file):
        print(f"✅ flash_project_args 存在")
        with open(flash_args_file, 'r') as f:
            content = f.read()
            if 'model' in content.lower() or '0x410000' in content or '0x800000' in content:
                print(f"   ✅ 包含 model 分区烧录配置")
                print(f"   内容预览:")
                for line in content.split('\n'):
                    if 'model' in line.lower() or '0x410000' in line or '0x800000' in line or 'srmodels' in line:
                        print(f"      {line}")
            else:
                print(f"   ❌ 未找到 model 分区烧录配置")
                print(f"   完整内容:")
                print(content)
    else:
        print(f"❌ flash_project_args 不存在")
    
    print()

def check_sdkconfig():
    """检查 sdkconfig"""
    print("=" * 60)
    print("🔍 检查 sdkconfig 配置")
    print("=" * 60)
    
    if os.path.exists("sdkconfig"):
        print("✅ sdkconfig 存在")
        with open("sdkconfig", 'r') as f:
            for line in f:
                if 'PARTITION_TABLE' in line or 'VADN' in line or 'NSN' in line or 'MODEL' in line:
                    print(f"   {line.strip()}")
    
    print()

def main():
    os.chdir("/Users/machenyang/Desktop/xiaozhi-esp32-main")
    
    check_partition_table()
    check_srmodels_bin()
    check_flash_args()
    check_sdkconfig()
    
    print("=" * 60)
    print("📝 建议:")
    print("=" * 60)
    print("1. 如果 flash_project_args 中没有 model 分区，请执行:")
    print("   idf.py reconfigure")
    print("2. 然后重新编译:")
    print("   idf.py build")
    print("3. 最后烧录:")
    print("   idf.py flash")
    print()

if __name__ == "__main__":
    main()


