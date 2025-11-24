#!/bin/bash
python3 -c "
import struct
with open('build/srmodels/srmodels.bin', 'rb') as f:
    data = f.read(4)
    model_num = struct.unpack('I', data)[0]
    print(f'模型数量: {model_num}')
    print('')
    for i in range(model_num):
        model_name_bytes = f.read(32)
        model_name = model_name_bytes.split(b'\x00')[0].decode('utf-8')
        print(f'模型 {i}: {model_name}')
        file_num_bytes = f.read(4)
        file_num = struct.unpack('I', file_num_bytes)[0]
        f.seek(file_num * (32 + 4 + 4), 1)
"


