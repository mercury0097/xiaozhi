#!/usr/bin/env python3
"""
极简版小智服务器 - 使用硅基流动
只需要 openai, websockets, edge_tts 三个依赖
"""

import asyncio
import json
import struct
import subprocess
import tempfile
import os
from openai import OpenAI

# ========== 配置 ==========
SILICONFLOW_API_KEY = "sk-bpaekjqvzideexyhsacrcmmeyyxqlyeblsrktviihmkskyaw"
SILICONFLOW_BASE_URL = "https://api.siliconflow.cn/v1"
MODEL_NAME = "Qwen/Qwen2.5-7B-Instruct"  # 可以换成其他模型
SERVER_PORT = 8000

SYSTEM_PROMPT = """你是一个智能语音助手，名叫小智。你性格活泼可爱，说话简洁有趣。
请用简短的语句回复用户，不要太长，控制在50字以内。"""

# ========== 初始化 ==========
client = OpenAI(api_key=SILICONFLOW_API_KEY, base_url=SILICONFLOW_BASE_URL)

async def text_to_speech(text):
    """使用 edge-tts 进行语音合成"""
    import edge_tts
    
    communicate = edge_tts.Communicate(text, "zh-CN-XiaoxiaoNeural")
    
    # 创建临时文件
    with tempfile.NamedTemporaryFile(suffix=".mp3", delete=False) as f:
        temp_path = f.name
    
    await communicate.save(temp_path)
    
    # 读取音频数据
    with open(temp_path, "rb") as f:
        audio_data = f.read()
    
    os.unlink(temp_path)
    return audio_data

def chat_with_llm(user_message, history=[]):
    """调用硅基流动 LLM"""
    messages = [{"role": "system", "content": SYSTEM_PROMPT}]
    messages.extend(history)
    messages.append({"role": "user", "content": user_message})
    
    try:
        response = client.chat.completions.create(
            model=MODEL_NAME,
            messages=messages,
            max_tokens=200,
            temperature=0.7
        )
        return response.choices[0].message.content
    except Exception as e:
        print(f"LLM 错误: {e}")
        return "抱歉，我遇到了一些问题。"

async def handle_client(websocket):
    """处理 WebSocket 客户端连接"""
    print(f"客户端连接: {websocket.remote_address}")
    history = []
    
    try:
        async for message in websocket:
            if isinstance(message, str):
                # JSON 消息
                data = json.loads(message)
                msg_type = data.get("type", "")
                
                if msg_type == "hello":
                    # 回复 hello
                    response = {
                        "type": "hello",
                        "transport": "websocket",
                        "session_id": "simple-session",
                        "audio_params": {
                            "sample_rate": 24000,
                            "frame_duration": 60
                        }
                    }
                    await websocket.send(json.dumps(response))
                    print("已发送 hello 响应")
                    
                elif msg_type == "listen":
                    # 开始监听
                    state = data.get("state", "")
                    if state == "start":
                        print("开始监听...")
                    elif state == "stop":
                        print("停止监听")
                        
                elif msg_type == "stt":
                    # 语音识别结果
                    text = data.get("text", "")
                    if text:
                        print(f"用户说: {text}")
                        
                        # 调用 LLM
                        reply = chat_with_llm(text, history)
                        print(f"AI 回复: {reply}")
                        
                        # 更新历史
                        history.append({"role": "user", "content": text})
                        history.append({"role": "assistant", "content": reply})
                        if len(history) > 10:
                            history = history[-10:]
                        
                        # 发送文本回复
                        await websocket.send(json.dumps({
                            "type": "tts",
                            "state": "start",
                            "text": reply
                        }))
                        
                        # 生成语音
                        try:
                            audio_data = await text_to_speech(reply)
                            # 发送音频数据
                            await websocket.send(audio_data)
                        except Exception as e:
                            print(f"TTS 错误: {e}")
                        
                        # 发送结束
                        await websocket.send(json.dumps({
                            "type": "tts",
                            "state": "stop"
                        }))
                        
            else:
                # 二进制消息 (音频数据)
                # 这里简化处理，实际需要 ASR
                pass
                
    except Exception as e:
        print(f"连接错误: {e}")
    finally:
        print(f"客户端断开: {websocket.remote_address}")

async def main():
    import websockets
    
    print(f"""
╔══════════════════════════════════════════════════╗
║     极简版小智服务器 - 使用硅基流动              ║
╠══════════════════════════════════════════════════╣
║  模型: {MODEL_NAME:<40} ║
║  端口: {SERVER_PORT:<40} ║
╠══════════════════════════════════════════════════╣
║  WebSocket 地址:                                 ║
║  ws://0.0.0.0:{SERVER_PORT}/xiaozhi/v1/                    ║
╚══════════════════════════════════════════════════╝
""")
    
    async with websockets.serve(handle_client, "0.0.0.0", SERVER_PORT):
        print("服务器已启动，等待连接...")
        await asyncio.Future()  # 永远运行

if __name__ == "__main__":
    asyncio.run(main())
