#include "afe_audio_processor.h"
#include <esp_log.h>
#include <string.h>
#include <esp_partition.h>

#define PROCESSOR_RUNNING 0x01

#define TAG "AfeAudioProcessor"

AfeAudioProcessor::AfeAudioProcessor()
    : afe_data_(nullptr) {
    event_group_ = xEventGroupCreate();
}

void AfeAudioProcessor::Initialize(AudioCodec* codec, int frame_duration_ms, srmodel_list_t* models_list) {
    codec_ = codec;
    frame_samples_ = frame_duration_ms * 16000 / 1000;

    // Pre-allocate output buffer capacity
    output_buffer_.reserve(frame_samples_);

    int ref_num = codec_->input_reference() ? 1 : 0;

    std::string input_format;
    for (int i = 0; i < codec_->input_channels() - ref_num; i++) {
        input_format.push_back('M');
    }
    for (int i = 0; i < ref_num; i++) {
        input_format.push_back('R');
    }

    // 🎯 直接读取 Flash 分区验证数据
    ESP_LOGI(TAG, "🔍 直接读取 model 分区验证烧录是否成功...");
    const esp_partition_t* model_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "model");
    if (model_part != nullptr) {
        uint8_t header[256];
        if (esp_partition_read(model_part, 0, header, sizeof(header)) == ESP_OK) {
            int model_count = (int)(header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24));
            ESP_LOGI(TAG, "📦 Flash 中的模型数量: %d", model_count);
            
            // 读取模型名称（从偏移 4 开始，每个模型名称占 32 字节）
            for (int i = 0; i < model_count && i < 10; i++) {
                char model_name[33] = {0};
                size_t offset = 4 + i * 256;  // 粗略估计
                if (offset + 32 < sizeof(header)) {
                    memcpy(model_name, &header[offset], 32);
                    model_name[32] = '\0';
                    if (strlen(model_name) > 0) {
                        ESP_LOGI(TAG, "   Flash 模型 %d: %s", i, model_name);
                    }
                }
            }
        }
    }
    
    // 强制重新加载模型
    ESP_LOGI(TAG, "🔍 强制从 model 分区重新加载模型（忽略可能的缓存）...");
    
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == nullptr) {
        ESP_LOGE(TAG, "❌ 从 model 分区加载模型失败！");
        return;
    }
    
    ESP_LOGI(TAG, "✅ ESP-SR 加载的模型数量: %d", models->num);
    // 打印所有模型名称
    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "   ESP-SR 模型 %d: %s", i, models->model_name[i]);
    }

    // 🎯 显式指定使用神经网络模型（避免 filter 返回旧版本）
    #ifdef CONFIG_SR_NSN_NSNET2
        char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, "nsnet2");
        ESP_LOGI(TAG, "🔍 指定降噪模型: nsnet2 (神经网络)");
    #else
        char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    #endif
    
    #ifdef CONFIG_SR_VADN_VADNET1_MEDIUM
        // 🎯 尝试多个可能的 VADNet1 模型名称（ESP-IDF 5.5 可能使用不同的名称）
        char* vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadnet1_medium");
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadnet1_medium，尝试 vadnet1...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadnet1");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadnet1，尝试 vadn1_medium...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadn1_medium");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到 vadn1_medium，尝试 vadn1...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, "vadn1");
        }
        if (vad_model_name == nullptr) {
            ESP_LOGW(TAG, "⚠️  未找到任何 VADNet 模型，尝试不指定名称（使用第一个匹配的）...");
            vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);
        }

        // 🎯 打印所有可用的 VAD 模型，帮助调试
        ESP_LOGI(TAG, "🔍 可用 VAD 模型列表:");
        for (int i = 0; i < models->num; i++) {
            if (strstr(models->model_name[i], "vad") != NULL) {
                ESP_LOGI(TAG, "   [%d] %s", i, models->model_name[i]);
            }
        }

        ESP_LOGI(TAG, "🔍 指定 VAD 模型: vadnet1_medium (神经网络), 找到: %s",
                 vad_model_name ? vad_model_name : "NULL");
    #else
        char* vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);
        ESP_LOGI(TAG, "🔍 VAD 模型: 使用默认 (WebRTC)");
    #endif
    
    // 使用低功耗模式，避免 CPU 过载
    // 🎯 传入 models 以确保使用我们指定的 nsnet2 和 vadnet1_medium
    afe_config_t* afe_config = afe_config_init(input_format.c_str(), models, AFE_TYPE_VC, AFE_MODE_LOW_COST);
    // 🛡️ 使用 SR_LOW_COST 模式的 AEC，VOIP 模式太耗 CPU 会触发看门狗
    afe_config->aec_mode = AEC_MODE_SR_LOW_COST;
    
    // 🎯 优化 VAD 参数以更好地检测人声，减少识别延迟
    afe_config->vad_mode = VAD_MODE_3;  // 最灵敏模式（0=不灵敏, 3=灵敏）
    afe_config->vad_min_noise_ms = 10;   // 进一步缩短噪声判断时间（从30ms降到10ms）
    if (vad_model_name != nullptr) {
        afe_config->vad_model_name = vad_model_name;
        ESP_LOGI(TAG, "✅ VAD 人声检测: 神经网络模式 (%s, Level 3 高灵敏)", vad_model_name);
    } else {
        ESP_LOGI(TAG, "✅ VAD 人声检测: WebRTC 模式 (Level 3 高灵敏)");
    }

    if (ns_model_name != nullptr) {
        // 使用神经网络降噪模型（现在会被打包）
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
        ESP_LOGI(TAG, "✅ 使用 ESP-SR 神经网络降噪: %s", ns_model_name);
        ESP_LOGI(TAG, "   降噪模式: AFE_NS_MODE_NET (%d)", (int)afe_config->afe_ns_mode);
    } else {
        // 没有 NS 模型，禁用降噪
        afe_config->ns_init = false;
        ESP_LOGW(TAG, "⚠️  未找到 NS 降噪模型，降噪已禁用");
        ESP_LOGW(TAG, "   请运行: idf.py build (会自动打包 NS 模型)");
    }

    // 🎯 启用 AGC（自动增益控制）增强人声
    afe_config->agc_init = true;
    afe_config->agc_mode = AFE_AGC_MODE_WEBRTC;  // 使用 WEBRTC AGC
    afe_config->agc_compression_gain_db = 15;     // 压缩增益 15dB（默认9，越大越激进）
    afe_config->agc_target_level_dbfs = 3;        // 目标电平 -3dBFS（默认3）
    ESP_LOGI(TAG, "✅ AGC 自动增益控制: WEBRTC 模式 (增益=15dB)");
    
    // 🎯 启用 SE（语音增强）突出人声频段
    afe_config->se_init = true;
    ESP_LOGI(TAG, "✅ SE 语音增强: 已启用（突出人声频段，抑制音乐）");
    
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    
    // 🎯 减小 AFE Ringbuffer，降低延迟（之前太大导致延迟高）
    // 把 AFE 任务移到 CPU0 后，不再需要这么大的缓冲
    afe_config->afe_ringbuf_size = 1000;  // 减小到 1000（约 62ms 缓冲）
    
    // 🎯 AFE 内部任务配置 - 移到 CPU0 避免与音频播放竞争
    // CPU0: audio_input + AFE 处理（输入链路）
    // CPU1: audio_output + opus_codec（输出链路）
    afe_config->afe_perferred_core = 0;   // 移到 CPU0，与 audio_input 同核
    afe_config->afe_perferred_priority = 6; // 优先级 6，高于 audio_input(5)

    // 🎯 启用 AEC（回声消除）+ VAD（语音检测）
    // 同时启用 AEC 和 VAD，AEC 消除播放音频的回声，VAD 检测人声
    afe_config->aec_init = codec_->input_reference();  // 有参考通道才启用 AEC
    afe_config->vad_init = true;  // 始终启用 VAD 用于人声检测
    
    if (afe_config->aec_init) {
        ESP_LOGI(TAG, "✅ AEC 回声消除: 已启用（VOIP_LOW_COST 模式）");
    } else {
        ESP_LOGI(TAG, "ℹ️  AEC 回声消除: 未启用（需要参考音频通道）");
    }

    // 🎯 显示优化后的配置
    ESP_LOGI(TAG, "   Ringbuffer 大小: %d, AFE 优先级: %d, AFE 核心: %d",
             afe_config->afe_ringbuf_size, afe_config->afe_perferred_priority, afe_config->afe_perferred_core);

    afe_iface_ = esp_afe_handle_from_config(afe_config);
    afe_data_ = afe_iface_->create_from_config(afe_config);
    
    // 🎯 audio_communication 任务 - 移到 CPU0，与 AFE 内部任务同核
    // CPU0 任务链：audio_input(5) → AFE内部(6) → audio_communication(6)
    // 这样输入链路在同一个核心，避免跨核通信延迟
    xTaskCreatePinnedToCore([](void* arg) {
        auto this_ = (AfeAudioProcessor*)arg;
        this_->AudioProcessorTask();
        vTaskDelete(NULL);
    }, "audio_communication", 4096 * 2, this, 6, NULL, 0);  // 改为 CPU0
}

AfeAudioProcessor::~AfeAudioProcessor() {
    if (afe_data_ != nullptr) {
        afe_iface_->destroy(afe_data_);
    }
    vEventGroupDelete(event_group_);
}

size_t AfeAudioProcessor::GetFeedSize() {
    if (afe_data_ == nullptr) {
        return 0;
    }
    return afe_iface_->get_feed_chunksize(afe_data_);
}

void AfeAudioProcessor::Feed(std::vector<int16_t>&& data) {
    if (afe_data_ == nullptr) {
        return;
    }
    // 检查是否正在运行，避免 Stop 后继续 feed 导致 ringbuffer 溢出
    if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
        return;
    }
    // 🎯 喂数据前让出 CPU，给 fetch 任务和渲染任务处理时间
    taskYIELD();
    afe_iface_->feed(afe_data_, data.data());
    // 🎯 喂完数据后再让出一次，确保 fetch 有机会处理
    taskYIELD();
}

void AfeAudioProcessor::Start() {
    xEventGroupSetBits(event_group_, PROCESSOR_RUNNING);
}

void AfeAudioProcessor::Stop() {
    xEventGroupClearBits(event_group_, PROCESSOR_RUNNING);
    if (afe_data_ != nullptr) {
        afe_iface_->reset_buffer(afe_data_);
    }
}

bool AfeAudioProcessor::IsRunning() {
    return xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING;
}

void AfeAudioProcessor::OnOutput(std::function<void(std::vector<int16_t>&& data)> callback) {
    output_callback_ = callback;
}

void AfeAudioProcessor::OnVadStateChange(std::function<void(bool speaking)> callback) {
    vad_state_change_callback_ = callback;
}

void AfeAudioProcessor::AudioProcessorTask() {
    auto fetch_size = afe_iface_->get_fetch_chunksize(afe_data_);
    auto feed_size = afe_iface_->get_feed_chunksize(afe_data_);
    ESP_LOGI(TAG, "Audio communication task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (true) {
        xEventGroupWaitBits(event_group_, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);

        auto res = afe_iface_->fetch_with_delay(afe_data_, portMAX_DELAY);
        if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
            continue;
        }
        if (res == nullptr || res->ret_value == ESP_FAIL) {
            if (res != nullptr) {
                ESP_LOGI(TAG, "Error code: %d", res->ret_value);
            }
            continue;
        }

        // VAD state change
        if (vad_state_change_callback_) {
            if (res->vad_state == VAD_SPEECH && !is_speaking_) {
                is_speaking_ = true;
                vad_state_change_callback_(true);
            } else if (res->vad_state == VAD_SILENCE && is_speaking_) {
                is_speaking_ = false;
                vad_state_change_callback_(false);
            }
        }

        if (output_callback_) {
            size_t samples = res->data_size / sizeof(int16_t);
            
            // Add data to buffer
            output_buffer_.insert(output_buffer_.end(), res->data, res->data + samples);
            
            // Output complete frames when buffer has enough data
            while (output_buffer_.size() >= frame_samples_) {
                if (output_buffer_.size() == frame_samples_) {
                    // If buffer size equals frame size, move the entire buffer
                    output_callback_(std::move(output_buffer_));
                    output_buffer_.clear();
                    output_buffer_.reserve(frame_samples_);
                } else {
                    // If buffer size exceeds frame size, copy one frame and remove it
                    output_callback_(std::vector<int16_t>(output_buffer_.begin(), output_buffer_.begin() + frame_samples_));
                    output_buffer_.erase(output_buffer_.begin(), output_buffer_.begin() + frame_samples_);
                }
            }
        }
    }
}

void AfeAudioProcessor::EnableDeviceAec(bool enable) {
    if (enable) {
#if CONFIG_USE_DEVICE_AEC
        afe_iface_->disable_vad(afe_data_);
        afe_iface_->enable_aec(afe_data_);
#else
        ESP_LOGE(TAG, "Device AEC is not supported");
#endif
    } else {
        afe_iface_->disable_aec(afe_data_);
        afe_iface_->enable_vad(afe_data_);
    }
}
