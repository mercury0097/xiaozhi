# 🎤 语音交互与 AI 通信完整流程详解

> 本文档详细讲解 Otto 机器人从接收语音到与 AI 通信再到播放回复的完整流程，适合初学者阅读。

---

## 目录

1. [整体流程概览](#1-整体流程概览)
2. [第一步：唤醒词检测](#2-第一步唤醒词检测)
3. [第二步：建立连接](#3-第二步建立连接)
4. [第三步：录音并发送](#4-第三步录音并发送)
5. [第四步：服务器处理](#5-第四步服务器处理)
6. [第五步：接收并播放](#6-第五步接收并播放)
7. [完整时序图](#7-完整时序图)
8. [关键数据格式](#8-关键数据格式)
9. [设备状态机](#9-设备状态机)
10. [关键文件总结](#10-关键文件总结)

---

## 1. 整体流程概览

语音交互的完整流程可以分为 5 个主要步骤：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        语音交互完整流程                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐   │
│  │ 1.唤醒  │ →  │ 2.录音  │ →  │ 3.发送  │ →  │ 4.AI处理│ →  │ 5.播放  │   │
│  │         │    │         │    │         │    │         │    │         │   │
│  │"你好小智"│    │ 用户说话 │    │ 音频上传 │    │ 服务器  │    │ 机器人  │   │
│  │         │    │         │    │ 到服务器 │    │ 返回回复│    │ 说话    │   │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘    └─────────┘   │
│       │              │              │              │              │         │
│       ↓              ↓              ↓              ↓              ↓         │
│   WakeNet        AFE处理       Opus编码      WebSocket      Opus解码       │
│   检测唤醒词     降噪+VAD      压缩音频      双向通信       还原音频        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**简单理解：** 就像打电话一样 —— 你说话 → 对方听到 → 对方回复 → 你听到回复 🎤📞

---

## 2. 第一步：唤醒词检测

### 2.1 发生了什么？

机器人一直在"听"，等待你说"你好小智"。使用 WakeNet9 神经网络模型检测唤醒词。

### 2.2 工作原理

```
┌─────────────────────────────────────────────────────────────────┐
│                    唤醒词检测流程                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  麦克风 → I2S采集 → AFE预处理 → WakeNet神经网络 → 检测结果      │
│                         │              │                        │
│                         │              ↓                        │
│                    降噪+AEC      "你好小智" 匹配？               │
│                                        │                        │
│                                        ↓                        │
│                                   是 → 触发唤醒                  │
│                                   否 → 继续监听                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 代码实现

**文件位置：** `main/audio/wake_words/afe_wake_word.cc`

```cpp
// 唤醒词检测任务一直在后台运行
void AfeWakeWord::AudioDetectionTask() {
    while (true) {
        // 从 AFE 获取处理后的音频
        auto res = afe_iface_->fetch_with_delay(afe_data_, portMAX_DELAY);
        
        // 检测到唤醒词！
        if (res->wakeup_state == WAKENET_DETECTED) {
            // 获取唤醒词名称（如"你好小智"）
            last_detected_wake_word_ = wake_words_[res->wakenet_model_index - 1];
            
            // 停止检测，通知应用层
            Stop();
            if (wake_word_detected_callback_) {
                wake_word_detected_callback_(last_detected_wake_word_);
            }
        }
    }
}
```

### 2.4 唤醒词模型

| 配置项 | 值 | 说明 |
|--------|-----|------|
| 模型 | WakeNet9 | 神经网络唤醒词检测 |
| 唤醒词 | "你好小智" | 可配置其他唤醒词 |
| 检测阈值 | 0.48 | 越低越灵敏，但误触发也会增加 |

---

## 3. 第二步：建立连接

### 3.1 发生了什么？

检测到唤醒词后，设备通过 WebSocket 连接到 AI 服务器，发送"hello"消息告诉服务器设备的音频参数。

### 3.2 连接流程

```
设备                                    服务器
  │                                       │
  │ ──── WebSocket 连接 ────────────────→ │
  │                                       │
  │ ──── {"type":"hello",...} ──────────→ │  (告诉服务器音频参数)
  │                                       │
  │ ←──── {"type":"hello",...} ────────── │  (服务器确认)
  │                                       │
  │        连接建立成功！                  │
```

### 3.3 代码实现

**文件位置：** `main/application.cc`

```cpp
// 唤醒词检测后的处理
void Application::OnWakeWordDetected() {
    // 1. 如果还没连接，先建立连接
    if (!protocol_->IsAudioChannelOpened()) {
        SetDeviceState(kDeviceStateConnecting);  // 显示"连接中..."
        
        if (!protocol_->OpenAudioChannel()) {    // 建立 WebSocket 连接
            return;
        }
    }
    
    // 2. 发送唤醒词数据给服务器
    protocol_->SendWakeWordDetected(wake_word);
    
    // 3. 进入监听模式
    SetListeningMode(kListeningModeRealtime);
}
```

**文件位置：** `main/protocols/websocket_protocol.cc`

```cpp
// 建立 WebSocket 连接
bool WebsocketProtocol::OpenAudioChannel() {
    // 1. 创建 WebSocket 连接
    websocket_ = network->CreateWebSocket(1);
    websocket_->Connect(url.c_str());
    
    // 2. 发送 hello 消息
    auto message = GetHelloMessage();
    SendText(message);
    
    // 3. 等待服务器回复
    xEventGroupWaitBits(..., WEBSOCKET_PROTOCOL_SERVER_HELLO_EVENT, ...);
    
    return true;
}

// hello 消息内容
std::string WebsocketProtocol::GetHelloMessage() {
    // 告诉服务器：我用 Opus 格式，16kHz 采样率，单声道
    return R"({
        "type": "hello",
        "version": 3,
        "audio_params": {
            "format": "opus",
            "sample_rate": 16000,
            "channels": 1,
            "frame_duration": 60
        }
    })";
}
```

---

## 4. 第三步：录音并发送

### 4.1 发生了什么？

1. 麦克风采集你的声音
2. AFE 处理（降噪、VAD 检测）
3. Opus 编码压缩
4. 通过 WebSocket 发送到服务器

### 4.2 数据流

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         录音并发送数据流                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  麦克风 → I2S → AFE处理 → Opus编码 → WebSocket → 服务器                     │
│            │       │          │                                             │
│            │       │          │                                             │
│            ↓       ↓          ↓                                             │
│       PCM 16kHz  降噪后    压缩后                                            │
│       32KB/秒    32KB/秒   2-3KB/秒                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 AFE 处理

AFE (Audio Front-End) 对原始音频进行预处理：

| 模块 | 作用 | 当前配置 |
|------|------|----------|
| **AEC** | 回声消除，去除扬声器声音 | SR_LOW_COST 模式 |
| **NS** | 降噪，去除环境噪声 | NSNet2 神经网络 |
| **VAD** | 语音检测，判断是否在说话 | VADNet1 神经网络 |
| **AGC** | 自动增益，音量归一化 | WebRTC 模式 |

### 4.4 代码实现

**文件位置：** `main/audio/audio_service.cc`

```cpp
// 音频处理流程
void AudioService::Initialize(...) {
    // 1. AFE 处理后的音频输出回调
    audio_processor_->OnOutput([this](std::vector<int16_t>&& data) {
        // 把音频放入编码队列
        PushTaskToEncodeQueue(kAudioTaskTypeEncodeToSendQueue, std::move(data));
    });
    
    // 2. VAD 状态变化回调
    audio_processor_->OnVadStateChange([this](bool speaking) {
        voice_detected_ = speaking;  // 记录是否在说话
    });
}

// Opus 编码任务
void AudioService::OpusCodecTask() {
    while (true) {
        // 从编码队列取出 PCM 音频
        auto task = audio_encode_queue_.front();
        
        // Opus 编码（压缩）- 32KB/秒 → 2-3KB/秒
        opus_encoder_->Encode(std::move(task->pcm), packet->payload);
        
        // 放入发送队列
        audio_send_queue_.push_back(std::move(packet));
        
        // 通知主循环发送
        callbacks_.on_send_queue_available();
    }
}
```

**文件位置：** `main/application.cc`

```cpp
// 主循环发送音频
void Application::MainEventLoop() {
    while (true) {
        auto bits = xEventGroupWaitBits(...);
        
        // 有音频要发送
        if (bits & MAIN_EVENT_SEND_AUDIO) {
            while (auto packet = audio_service_.PopPacketFromSendQueue()) {
                // 通过 WebSocket 发送
                protocol_->SendAudio(std::move(packet));
            }
        }
    }
}
```

### 4.5 Opus 编码

Opus 是一种高效的音频压缩格式：

| 参数 | 值 | 说明 |
|------|-----|------|
| 采样率 | 16000 Hz | 语音识别标准采样率 |
| 通道数 | 1 | 单声道 |
| 帧时长 | 60 ms | 每帧音频时长 |
| 压缩比 | 约 10:1 | 32KB/秒 → 2-3KB/秒 |

---

## 5. 第四步：服务器处理

### 5.1 发生了什么？

服务器接收到音频后，进行以下处理：

```
┌─────────────────────────────────────────────────────────────────┐
│                    服务器处理流程                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  接收 Opus 音频                                                  │
│       │                                                         │
│       ↓                                                         │
│  ┌─────────────────┐                                            │
│  │  语音识别 (ASR) │  音频 → 文字                                │
│  │  "今天天气怎么样"│                                            │
│  └────────┬────────┘                                            │
│           ↓                                                     │
│  ┌─────────────────┐                                            │
│  │  大语言模型(LLM)│  理解意图，生成回复                         │
│  │  "今天天气晴朗" │                                            │
│  └────────┬────────┘                                            │
│           ↓                                                     │
│  ┌─────────────────┐                                            │
│  │  语音合成 (TTS) │  文字 → 音频                                │
│  │  Opus 音频流    │                                            │
│  └────────┬────────┘                                            │
│           ↓                                                     │
│  返回给设备                                                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 服务器返回的消息类型

服务器通过 WebSocket 返回多种类型的消息：

```json
// 1. 语音识别结果（你说的话）
{"type": "stt", "text": "今天天气怎么样"}

// 2. AI 开始回复
{"type": "tts", "state": "start"}

// 3. AI 回复的每一句话（用于显示字幕）
{"type": "tts", "state": "sentence_start", "text": "今天天气晴朗"}

// 4. AI 的表情（用于显示表情）
{"type": "llm", "emotion": "happy"}

// 5. AI 回复结束
{"type": "tts", "state": "stop"}

// 6. 音频数据（二进制 Opus 格式）
[二进制音频数据]
```

---

## 6. 第五步：接收并播放

### 6.1 发生了什么？

1. 接收服务器返回的 Opus 音频
2. Opus 解码还原为 PCM
3. 通过扬声器播放

### 6.2 数据流

```
服务器 → WebSocket → Opus解码 → PCM → I2S → 扬声器
                        │
                        │ 还原为 16kHz/24kHz PCM
                        │ 2-3KB/秒 → 32-48KB/秒
```

### 6.3 代码实现

**文件位置：** `main/application.cc`

```cpp
// 注册接收回调
void Application::Start() {
    // 接收音频数据（二进制）
    protocol_->OnIncomingAudio([this](std::unique_ptr<AudioStreamPacket> packet) {
        if (device_state_ == kDeviceStateSpeaking) {
            // 放入解码队列
            audio_service_.PushPacketToDecodeQueue(std::move(packet));
        }
    });
    
    // 接收 JSON 消息
    protocol_->OnIncomingJson([this](const cJSON* root) {
        auto type = cJSON_GetObjectItem(root, "type");
        
        // TTS 状态消息
        if (strcmp(type->valuestring, "tts") == 0) {
            auto state = cJSON_GetObjectItem(root, "state");
            
            if (strcmp(state->valuestring, "start") == 0) {
                // AI 开始说话
                SetDeviceState(kDeviceStateSpeaking);
            }
            else if (strcmp(state->valuestring, "sentence_start") == 0) {
                // 显示 AI 说的话（字幕）
                auto text = cJSON_GetObjectItem(root, "text");
                display->SetChatMessage("assistant", text->valuestring);
            }
            else if (strcmp(state->valuestring, "stop") == 0) {
                // AI 说完了，回到监听状态
                SetDeviceState(kDeviceStateListening);
            }
        }
        // 语音识别结果
        else if (strcmp(type->valuestring, "stt") == 0) {
            // 显示用户说的话
            auto text = cJSON_GetObjectItem(root, "text");
            display->SetChatMessage("user", text->valuestring);
        }
        // 表情
        else if (strcmp(type->valuestring, "llm") == 0) {
            // 设置机器人表情
            auto emotion = cJSON_GetObjectItem(root, "emotion");
            display->SetEmotion(emotion->valuestring);
        }
    });
}
```

**文件位置：** `main/audio/audio_service.cc`

```cpp
// 解码任务
void AudioService::OpusCodecTask() {
    while (true) {
        // 从解码队列取出 Opus 数据
        auto packet = audio_decode_queue_.front();
        
        // Opus 解码（还原）
        opus_decoder_->Decode(std::move(packet->payload), task->pcm);
        
        // 音量增益处理（提升 1.5 倍）
        for (auto& sample : task->pcm) {
            int32_t amplified = static_cast<int32_t>(sample * 1.5f);
            // 削波保护
            sample = std::clamp(amplified, -32768, 32767);
        }
        
        // 放入播放队列
        audio_playback_queue_.push_back(std::move(task));
    }
}

// 播放任务
void AudioService::AudioOutputTask() {
    while (true) {
        // 从播放队列取出 PCM 数据
        auto task = audio_playback_queue_.front();
        
        // 输出到扬声器
        codec_->OutputData(task->pcm);
    }
}
```

---

## 7. 完整时序图

```
时间 ─────────────────────────────────────────────────────────────────────→

用户        设备                    网络                    服务器
 │           │                       │                       │
 │ "你好小智" │                       │                       │
 │ ─────────→│                       │                       │
 │           │ WakeNet检测到唤醒词    │                       │
 │           │                       │                       │
 │           │ ══════ WebSocket 连接 ══════════════════════→ │
 │           │                       │                       │
 │           │ ────── hello 消息 ──────────────────────────→ │
 │           │ ←───── hello 回复 ────────────────────────── │
 │           │                       │                       │
 │           │ 显示"正在听..."        │                       │
 │           │                       │                       │
 │ "今天天气" │                       │                       │
 │ ─────────→│                       │                       │
 │           │ AFE处理 → Opus编码    │                       │
 │           │ ────── 音频数据 ────────────────────────────→ │
 │           │ ────── 音频数据 ────────────────────────────→ │
 │           │ ────── 音频数据 ────────────────────────────→ │
 │           │                       │                       │
 │           │                       │        语音识别(ASR)   │
 │           │                       │        LLM 生成回复    │
 │           │                       │        语音合成(TTS)   │
 │           │                       │                       │
 │           │ ←───── stt: "今天天气" ────────────────────── │
 │           │ 显示用户说的话         │                       │
 │           │                       │                       │
 │           │ ←───── tts: start ─────────────────────────── │
 │           │ 切换到"说话"状态       │                       │
 │           │                       │                       │
 │           │ ←───── 音频数据 ─────────────────────────────  │
 │           │ ←───── 音频数据 ─────────────────────────────  │
 │           │ Opus解码 → 扬声器播放  │                       │
 │           │                       │                       │
 │ ←─────────│ "今天天气晴朗..."      │                       │
 │  听到回复  │                       │                       │
 │           │                       │                       │
 │           │ ←───── tts: stop ──────────────────────────── │
 │           │ 回到"监听"状态         │                       │
 │           │                       │                       │
```

---

## 8. 关键数据格式

### 8.1 音频数据格式

| 阶段 | 格式 | 采样率 | 位深 | 数据量 |
|------|------|--------|------|--------|
| 麦克风采集 | PCM | 16kHz | 16bit | 32KB/秒 |
| AFE 处理后 | PCM | 16kHz | 16bit | 32KB/秒 |
| Opus 编码后 | Opus | - | - | 2-3KB/秒 |
| 服务器返回 | Opus | - | - | 2-3KB/秒 |
| 解码后播放 | PCM | 16/24kHz | 16bit | 32-48KB/秒 |

### 8.2 WebSocket 消息格式

#### 文本消息（JSON）

**设备 → 服务器：**
```json
// 握手消息
{"type": "hello", "version": 3, "audio_params": {"format": "opus", "sample_rate": 16000, "channels": 1}}

// 唤醒词检测
{"type": "listen", "state": "detect", "text": "你好小智"}

// 开始监听
{"type": "listen", "state": "start", "mode": "realtime"}

// 停止监听
{"type": "listen", "state": "stop"}

// 打断 AI 说话
{"type": "abort", "reason": "wake_word_detected"}
```

**服务器 → 设备：**
```json
// 握手回复
{"type": "hello", "session_id": "xxx", "audio_params": {"sample_rate": 24000}}

// 语音识别结果
{"type": "stt", "text": "用户说的话"}

// TTS 状态
{"type": "tts", "state": "start"}
{"type": "tts", "state": "sentence_start", "text": "AI说的话"}
{"type": "tts", "state": "stop"}

// 表情
{"type": "llm", "emotion": "happy"}
```

#### 二进制消息（音频）

```
┌──────────┬──────────┬──────────────┬─────────────────┐
│ type (1B)│reserved  │payload_size  │ Opus 音频数据   │
│    0     │ (1B)     │ (2B, 大端)   │ (变长)          │
└──────────┴──────────┴──────────────┴─────────────────┘
```

---

## 9. 设备状态机

设备在语音交互过程中会经历以下状态：

```
┌─────────────────────────────────────────────────────────────────┐
│                        设备状态机                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                    ┌──────────┐                                 │
│                    │  Idle    │ ← 待机状态（等待唤醒词）          │
│                    │ (待机)   │                                 │
│                    └────┬─────┘                                 │
│                         │ 检测到唤醒词                           │
│                         ↓                                       │
│                    ┌──────────┐                                 │
│                    │Connecting│ ← 连接服务器中                   │
│                    │ (连接中) │                                 │
│                    └────┬─────┘                                 │
│                         │ 连接成功                               │
│                         ↓                                       │
│     ┌──────────────────────────────────────┐                   │
│     │                                      │                   │
│     ↓                                      │                   │
│ ┌──────────┐                          ┌──────────┐             │
│ │Listening │ ←────────────────────────│ Speaking │             │
│ │ (监听中) │      AI说完了            │ (说话中) │             │
│ └────┬─────┘                          └────┬─────┘             │
│      │                                     ↑                   │
│      │ 用户说完了                          │ AI开始回复         │
│      └─────────────────────────────────────┘                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 状态说明

| 状态 | 英文 | 说明 | 显示 |
|------|------|------|------|
| 待机 | Idle | 等待唤醒词，低功耗 | "待机" |
| 连接中 | Connecting | 正在连接服务器 | "连接中..." |
| 监听中 | Listening | 正在录音，等待用户说话 | "正在听..." |
| 说话中 | Speaking | 正在播放 AI 回复 | 显示 AI 回复文字 |

---

## 10. 关键文件总结

### 10.1 核心文件列表

| 文件 | 路径 | 作用 |
|------|------|------|
| **application.cc** | `main/application.cc` | 主应用逻辑，状态机控制，事件处理 |
| **audio_service.cc** | `main/audio/audio_service.cc` | 音频服务，管理录音/播放/编解码 |
| **afe_audio_processor.cc** | `main/audio/processors/afe_audio_processor.cc` | AFE 音频处理（降噪、VAD） |
| **afe_wake_word.cc** | `main/audio/wake_words/afe_wake_word.cc` | 唤醒词检测 |
| **websocket_protocol.cc** | `main/protocols/websocket_protocol.cc` | WebSocket 通信协议 |
| **protocol.cc** | `main/protocols/protocol.cc` | 协议基类，消息发送 |

### 10.2 文件关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                        代码架构                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    application.cc                        │   │
│  │                    (主应用逻辑)                           │   │
│  └────────────────────────┬────────────────────────────────┘   │
│                           │                                     │
│           ┌───────────────┼───────────────┐                    │
│           │               │               │                    │
│           ↓               ↓               ↓                    │
│  ┌────────────────┐ ┌────────────┐ ┌────────────────┐         │
│  │ audio_service  │ │  protocol  │ │    display     │         │
│  │   (音频服务)   │ │  (通信协议) │ │   (显示控制)   │         │
│  └───────┬────────┘ └─────┬──────┘ └────────────────┘         │
│          │                │                                    │
│    ┌─────┴─────┐    ┌─────┴─────┐                             │
│    │           │    │           │                             │
│    ↓           ↓    ↓           ↓                             │
│ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                          │
│ │ AFE  │ │ Opus │ │ WS   │ │ MQTT │                          │
│ │处理器│ │编解码│ │协议  │ │协议  │                          │
│ └──────┘ └──────┘ └──────┘ └──────┘                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 10.3 任务分配

| 任务名 | 优先级 | CPU | 作用 |
|--------|--------|-----|------|
| audio_input | 5 | CPU0 | 从麦克风读取音频 |
| audio_output | 4 | CPU1 | 输出音频到扬声器 |
| opus_codec | 5 | - | Opus 编解码 |
| audio_communication | 4 | CPU1 | AFE 音频处理 |
| main_event_loop | 3 | - | 主事件循环 |
| audio_detection | 3 | - | 唤醒词检测 |

---

## 11. 常见问题

### Q1: 为什么要用 Opus 编码？

Opus 是一种高效的音频压缩格式，可以将 32KB/秒的 PCM 音频压缩到 2-3KB/秒，大大减少网络传输量，同时保持较好的音质。

### Q2: VAD 有什么用？

VAD (Voice Activity Detection) 用于检测用户是否在说话：
- 用户开始说话 → 开始录音
- 用户停止说话 → 停止录音，发送给服务器
- 节省带宽，只传输有语音的部分

### Q3: 为什么需要 AEC？

AEC (Acoustic Echo Cancellation) 用于消除扬声器的回声。当机器人在说话时，麦克风会采集到扬声器的声音，AEC 可以消除这部分回声，让服务器只听到用户的声音。

### Q4: WebSocket 和 MQTT 有什么区别？

- **WebSocket**：全双工通信，延迟低，适合实时语音交互
- **MQTT**：轻量级消息协议，适合物联网设备，但延迟稍高

项目默认使用 WebSocket，也支持 MQTT。

---

## 12. 总结

语音交互的核心流程：

1. **唤醒** → WakeNet 检测到"你好小智"
2. **连接** → WebSocket 连接到 AI 服务器
3. **录音** → 麦克风 → AFE 处理 → Opus 编码 → 发送
4. **AI 处理** → 服务器做语音识别 + LLM + 语音合成
5. **播放** → 接收 Opus → 解码 → 扬声器播放

整个过程就像打电话：你说话 → 对方听到 → 对方回复 → 你听到回复 🎤📞

---

## 参考资料

- [ESP-IDF WebSocket 文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_websocket_client.html)
- [Opus 编解码器](https://opus-codec.org/)
- [ESP-SR 语音识别框架](https://github.com/espressif/esp-sr)
- [语音输入与AFE预处理详解](./语音输入与AFE预处理详解.md)
