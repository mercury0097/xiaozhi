#!/bin/bash
# 验证设备上的 model 分区是否已烧录

echo "🔍 读取设备上 model 分区的前 1KB 数据..."
esptool.py --chip esp32s3 --port /dev/tty.usbmodem101 read_flash 0x800000 1024 /tmp/model_check.bin

echo ""
echo "🔍 检查读取的数据..."
if [ -f /tmp/model_check.bin ]; then
    SIZE=$(stat -f%z /tmp/model_check.bin)
    echo "✅ 读取成功，大小: $SIZE 字节"
    
    # 检查是否全是 0xFF (未烧录)
    if xxd /tmp/model_check.bin | grep -v "ffff ffff ffff ffff" > /dev/null; then
        echo "✅ model 分区有数据！"
        echo "数据预览:"
        xxd /tmp/model_check.bin | head -20
    else
        echo "❌ model 分区是空的 (全是 0xFF)，说明没有被烧录！"
        echo ""
        echo "📝 解决方案: 在 Mac 终端执行完整烧录:"
        echo "   . /Users/machenyang/Desktop/esp-5.51/v5.5.1/esp-idf/export.sh"
        echo "   cd /Users/machenyang/Desktop/xiaozhi-esp32-main"
        echo "   idf.py -p /dev/tty.usbmodem101 flash"
    fi
else
    echo "❌ 读取失败"
fi

rm -f /tmp/model_check.bin


