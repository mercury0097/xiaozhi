#include "no_audio_codec.h"

#include <cmath>
#include <cstring>
#include <esp_log.h>

#define TAG "NoAudioCodec"

NoAudioCodec::~NoAudioCodec() {
  if (rx_handle_ != nullptr) {
    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
  }
  if (tx_handle_ != nullptr) {
    ESP_ERROR_CHECK(i2s_channel_disable(tx_handle_));
  }
}

NoAudioCodecDuplex::NoAudioCodecDuplex(int input_sample_rate,
                                       int output_sample_rate, gpio_num_t bclk,
                                       gpio_num_t ws, gpio_num_t dout,
                                       gpio_num_t din) {
  duplex_ = true;
  input_sample_rate_ = input_sample_rate;
  output_sample_rate_ = output_sample_rate;

  i2s_chan_config_t chan_cfg = {
      .id = I2S_NUM_0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
      .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .intr_priority = 0,
  };
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_));

  i2s_std_config_t std_cfg = {
      .clk_cfg =
          {
              .sample_rate_hz = (uint32_t)output_sample_rate_,
              .clk_src = I2S_CLK_SRC_DEFAULT,
              .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
              .ext_clk_freq_hz = 0,
#endif

          },
      .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                   .slot_mode = I2S_SLOT_MODE_MONO,
                   .slot_mask = I2S_STD_SLOT_LEFT,
                   .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .ws_pol = false,
                   .bit_shift = true,
#ifdef I2S_HW_VERSION_2
                   .left_align = true,
                   .big_endian = false,
                   .bit_order_lsb = false
#endif

      },
      .gpio_cfg = {.mclk = I2S_GPIO_UNUSED,
                   .bclk = bclk,
                   .ws = ws,
                   .dout = dout,
                   .din = din,
                   .invert_flags = {
                       .mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
  ESP_LOGI(TAG, "Duplex channels created");
}

NoAudioCodecSimplex::NoAudioCodecSimplex(int input_sample_rate,
                                         int output_sample_rate,
                                         gpio_num_t spk_bclk, gpio_num_t spk_ws,
                                         gpio_num_t spk_dout,
                                         gpio_num_t mic_sck, gpio_num_t mic_ws,
                                         gpio_num_t mic_din) {
  duplex_ = false;
  input_sample_rate_ = input_sample_rate;
  output_sample_rate_ = output_sample_rate;

  // Create a new channel for speaker
  i2s_chan_config_t chan_cfg = {
      .id = (i2s_port_t)0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
      .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .intr_priority = 0,
  };
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, nullptr));

  i2s_std_config_t std_cfg = {
      .clk_cfg =
          {
              .sample_rate_hz = (uint32_t)output_sample_rate_,
              .clk_src = I2S_CLK_SRC_DEFAULT,
              .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
              .ext_clk_freq_hz = 0,
#endif

          },
      .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                   .slot_mode = I2S_SLOT_MODE_MONO,
                   .slot_mask = I2S_STD_SLOT_LEFT,
                   .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .ws_pol = false,
                   .bit_shift = true,
#ifdef I2S_HW_VERSION_2
                   .left_align = true,
                   .big_endian = false,
                   .bit_order_lsb = false
#endif

      },
      .gpio_cfg = {.mclk = I2S_GPIO_UNUSED,
                   .bclk = spk_bclk,
                   .ws = spk_ws,
                   .dout = spk_dout,
                   .din = I2S_GPIO_UNUSED,
                   .invert_flags = {
                       .mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));

  // Create a new channel for MIC
  chan_cfg.id = (i2s_port_t)1;
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));
  std_cfg.clk_cfg.sample_rate_hz = (uint32_t)input_sample_rate_;
  std_cfg.gpio_cfg.bclk = mic_sck;
  std_cfg.gpio_cfg.ws = mic_ws;
  std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.din = mic_din;
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
  ESP_LOGI(TAG, "Simplex channels created");
}

NoAudioCodecSimplex::NoAudioCodecSimplex(
    int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk,
    gpio_num_t spk_ws, gpio_num_t spk_dout, i2s_std_slot_mask_t spk_slot_mask,
    gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din,
    i2s_std_slot_mask_t mic_slot_mask) {
  duplex_ = false;
  input_sample_rate_ = input_sample_rate;
  output_sample_rate_ = output_sample_rate;

  // Create a new channel for speaker
  i2s_chan_config_t chan_cfg = {
      .id = (i2s_port_t)0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
      .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .intr_priority = 0,
  };
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, nullptr));

  i2s_std_config_t std_cfg = {
      .clk_cfg =
          {
              .sample_rate_hz = (uint32_t)output_sample_rate_,
              .clk_src = I2S_CLK_SRC_DEFAULT,
              .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
              .ext_clk_freq_hz = 0,
#endif

          },
      .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                   .slot_mode = I2S_SLOT_MODE_MONO,
                   .slot_mask = spk_slot_mask,
                   .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .ws_pol = false,
                   .bit_shift = true,
#ifdef I2S_HW_VERSION_2
                   .left_align = true,
                   .big_endian = false,
                   .bit_order_lsb = false
#endif

      },
      .gpio_cfg = {.mclk = I2S_GPIO_UNUSED,
                   .bclk = spk_bclk,
                   .ws = spk_ws,
                   .dout = spk_dout,
                   .din = I2S_GPIO_UNUSED,
                   .invert_flags = {
                       .mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));

  // Create a new channel for MIC
  chan_cfg.id = (i2s_port_t)1;
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));
  std_cfg.clk_cfg.sample_rate_hz = (uint32_t)input_sample_rate_;
  std_cfg.slot_cfg.slot_mask = mic_slot_mask;
  std_cfg.gpio_cfg.bclk = mic_sck;
  std_cfg.gpio_cfg.ws = mic_ws;
  std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.din = mic_din;
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
  ESP_LOGI(TAG, "Simplex channels created");
}

int NoAudioCodec::Write(const int16_t *data, int samples) {
  std::lock_guard<std::mutex> lock(data_if_mutex_);
  std::vector<int32_t> buffer(samples);

  // output_volume_: 0-100
  // 针对 MAX98357A 最大增益的优化方案
  double volume_scale = static_cast<double>(output_volume_) / 100.0;
  if (volume_scale > 0.0) {
    // 使用平方曲线，符合人耳对响度的感知特性
    volume_scale = pow(volume_scale, 2.0);

    // 额外衰减系数，补偿硬件功放最大增益（MAX98357A GAIN=18dB）
    // 0.18 ≈ 18% 输出，可根据实际效果调整（建议范围：0.15-0.25）
    volume_scale *= 0.18;
  }

  const double max_val = static_cast<double>(INT32_MAX);
  const double min_val = static_cast<double>(INT32_MIN);

  // 将 int16 转换为 int32（左移16位），然后应用音量
  for (int i = 0; i < samples; i++) {
    int32_t sample_32 = static_cast<int32_t>(data[i]) << 16;
    double scaled = static_cast<double>(sample_32) * volume_scale;

    // 防止溢出
    if (scaled > max_val) {
      buffer[i] = INT32_MAX;
    } else if (scaled < min_val) {
      buffer[i] = INT32_MIN;
    } else {
      buffer[i] = static_cast<int32_t>(scaled);
    }
  }

  size_t bytes_written;
  ESP_ERROR_CHECK(i2s_channel_write(tx_handle_, buffer.data(),
                                    samples * sizeof(int32_t), &bytes_written,
                                    portMAX_DELAY));
  return bytes_written / sizeof(int32_t);
}

int NoAudioCodec::Read(int16_t *dest, int samples) {
  size_t bytes_read;

  std::vector<int32_t> bit32_buffer(samples);
  if (i2s_channel_read(rx_handle_, bit32_buffer.data(),
                       samples * sizeof(int32_t), &bytes_read,
                       portMAX_DELAY) != ESP_OK) {
    ESP_LOGE(TAG, "Read Failed!");
    return 0;
  }

  samples = bytes_read / sizeof(int32_t);
  for (int i = 0; i < samples; i++) {
    int32_t value = bit32_buffer[i] >> 12;
    dest[i] = (value > INT16_MAX)    ? INT16_MAX
              : (value < -INT16_MAX) ? -INT16_MAX
                                     : (int16_t)value;
  }
  return samples;
}

// Delegating constructor: calls the main constructor with default slot mask
NoAudioCodecSimplexPdm::NoAudioCodecSimplexPdm(
    int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk,
    gpio_num_t spk_ws, gpio_num_t spk_dout, gpio_num_t mic_sck,
    gpio_num_t mic_din)
    : NoAudioCodecSimplexPdm(input_sample_rate, output_sample_rate, spk_bclk,
                             spk_ws, spk_dout, I2S_STD_SLOT_LEFT, mic_sck,
                             mic_din) {
  // All initialization is handled by the delegated constructor
}

NoAudioCodecSimplexPdm::NoAudioCodecSimplexPdm(
    int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk,
    gpio_num_t spk_ws, gpio_num_t spk_dout, i2s_std_slot_mask_t spk_slot_mask,
    gpio_num_t mic_sck, gpio_num_t mic_din) {
  duplex_ = false;
  input_sample_rate_ = input_sample_rate;
  output_sample_rate_ = output_sample_rate;

  // Create a new channel for speaker
  i2s_chan_config_t tx_chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)1, I2S_ROLE_MASTER);
  tx_chan_cfg.dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM;
  tx_chan_cfg.dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM;
  tx_chan_cfg.auto_clear_after_cb = true;
  tx_chan_cfg.auto_clear_before_cb = false;
  tx_chan_cfg.intr_priority = 0;
  ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_handle_, NULL));

  i2s_std_config_t tx_std_cfg = {
      .clk_cfg =
          {
              .sample_rate_hz = (uint32_t)output_sample_rate_,
              .clk_src = I2S_CLK_SRC_DEFAULT,
              .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
              .ext_clk_freq_hz = 0,
#endif

          },
      .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                   .slot_mode = I2S_SLOT_MODE_MONO,
                   .slot_mask = spk_slot_mask,
                   .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .ws_pol = false,
                   .bit_shift = true,
#ifdef I2S_HW_VERSION_2
                   .left_align = true,
                   .big_endian = false,
                   .bit_order_lsb = false
#endif

      },
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = spk_bclk,
              .ws = spk_ws,
              .dout = spk_dout,
              .din = I2S_GPIO_UNUSED,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &tx_std_cfg));
#if SOC_I2S_SUPPORTS_PDM_RX
  // Create a new channel for MIC in PDM mode
  i2s_chan_config_t rx_chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)0, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_handle_));
  i2s_pdm_rx_config_t pdm_rx_cfg = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG((uint32_t)input_sample_rate_),
      /* The data bit-width of PDM mode is fixed to 16 */
      .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                 I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .clk = mic_sck,
              .din = mic_din,

              .invert_flags =
                  {
                      .clk_inv = false,
                  },
          },
  };
  ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle_, &pdm_rx_cfg));
#else
  ESP_LOGE(TAG, "PDM is not supported");
#endif
  ESP_LOGI(TAG, "Simplex channels created");
}

int NoAudioCodecSimplexPdm::Read(int16_t *dest, int samples) {
  size_t bytes_read;

  // PDM 解调后的数据位宽为 16 位，直接读取到目标缓冲区
  if (i2s_channel_read(rx_handle_, dest, samples * sizeof(int16_t), &bytes_read,
                       portMAX_DELAY) != ESP_OK) {
    ESP_LOGE(TAG, "Read Failed!");
    return 0;
  }

  samples = bytes_read / sizeof(int16_t);
  if (input_gain_ > 0) {
    int gain_factor = (int)input_gain_;
    for (int i = 0; i < samples; i++) {
      int32_t amplified = dest[i] * gain_factor;
      dest[i] = (amplified > INT16_MAX)    ? INT16_MAX
                : (amplified < -INT16_MAX) ? -INT16_MAX
                                           : (int16_t)amplified;
    }
  }
  return samples;
}

// ============================================================================
// NoAudioCodecSimplexAec - 支持软件 AEC 参考信号的 Simplex 编解码器
// ============================================================================

NoAudioCodecSimplexAec::NoAudioCodecSimplexAec(
    int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk,
    gpio_num_t spk_ws, gpio_num_t spk_dout, gpio_num_t mic_sck,
    gpio_num_t mic_ws, gpio_num_t mic_din) {

  duplex_ = false;
  input_reference_ = true; // 启用参考信号
  input_channels_ = 2;     // 2 通道：麦克风 + 参考
  input_sample_rate_ = input_sample_rate;
  output_sample_rate_ = output_sample_rate;

  // 初始化参考信号缓冲区
  ref_buffer_.resize(kRefBufferSize, 0);

  ESP_LOGI(TAG, "NoAudioCodecSimplexAec: 软件 AEC 参考信号已启用");

  // Create a new channel for speaker
  i2s_chan_config_t chan_cfg = {
      .id = (i2s_port_t)0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
      .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .intr_priority = 0,
  };
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, nullptr));

  i2s_std_config_t std_cfg = {
      .clk_cfg =
          {
              .sample_rate_hz = (uint32_t)output_sample_rate_,
              .clk_src = I2S_CLK_SRC_DEFAULT,
              .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
              .ext_clk_freq_hz = 0,
#endif
          },
      .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                   .slot_mode = I2S_SLOT_MODE_MONO,
                   .slot_mask = I2S_STD_SLOT_LEFT,
                   .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
                   .ws_pol = false,
                   .bit_shift = true,
#ifdef I2S_HW_VERSION_2
                   .left_align = true,
                   .big_endian = false,
                   .bit_order_lsb = false
#endif
      },
      .gpio_cfg = {.mclk = I2S_GPIO_UNUSED,
                   .bclk = spk_bclk,
                   .ws = spk_ws,
                   .dout = spk_dout,
                   .din = I2S_GPIO_UNUSED,
                   .invert_flags = {
                       .mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));

  // Create a new channel for MIC
  chan_cfg.id = (i2s_port_t)1;
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));
  std_cfg.clk_cfg.sample_rate_hz = (uint32_t)input_sample_rate_;
  std_cfg.gpio_cfg.bclk = mic_sck;
  std_cfg.gpio_cfg.ws = mic_ws;
  std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.din = mic_din;
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));

  ESP_LOGI(
      TAG,
      "Simplex AEC channels created (input_channels=%d, input_reference=%d)",
      input_channels_, input_reference_);
}

int NoAudioCodecSimplexAec::Write(const int16_t *data, int samples) {
  // 保存原始数据到参考缓冲区（用于 AEC）
  {
    std::lock_guard<std::mutex> lock(ref_mutex_);
    for (int i = 0; i < samples; i++) {
      ref_buffer_[ref_write_pos_] = data[i];
      ref_write_pos_ = (ref_write_pos_ + 1) % kRefBufferSize;
    }
  }

  // 调用父类的 Write 函数进行实际播放
  std::lock_guard<std::mutex> lock(data_if_mutex_);
  std::vector<int32_t> buffer(samples);

  double volume_scale = static_cast<double>(output_volume_) / 100.0;
  if (volume_scale > 0.0) {
    volume_scale = pow(volume_scale, 2.0);
    volume_scale *= 0.18;
  }

  const double max_val = static_cast<double>(INT32_MAX);
  const double min_val = static_cast<double>(INT32_MIN);

  for (int i = 0; i < samples; i++) {
    int32_t sample_32 = static_cast<int32_t>(data[i]) << 16;
    double scaled = static_cast<double>(sample_32) * volume_scale;

    if (scaled > max_val) {
      buffer[i] = INT32_MAX;
    } else if (scaled < min_val) {
      buffer[i] = INT32_MIN;
    } else {
      buffer[i] = static_cast<int32_t>(scaled);
    }
  }

  size_t bytes_written;
  ESP_ERROR_CHECK(i2s_channel_write(tx_handle_, buffer.data(),
                                    samples * sizeof(int32_t), &bytes_written,
                                    portMAX_DELAY));
  return bytes_written / sizeof(int32_t);
}

int NoAudioCodecSimplexAec::Read(int16_t *dest, int samples) {
  // samples 是期望的总样本数（包含 2 通道）
  // 实际麦克风样本数 = samples / 2
  int mic_samples = samples / 2;

  // 读取麦克风数据
  std::vector<int32_t> bit32_buffer(mic_samples);
  size_t bytes_read;
  if (i2s_channel_read(rx_handle_, bit32_buffer.data(),
                       mic_samples * sizeof(int32_t), &bytes_read,
                       portMAX_DELAY) != ESP_OK) {
    ESP_LOGE(TAG, "Read Failed!");
    return 0;
  }

  int actual_mic_samples = bytes_read / sizeof(int32_t);

  // 交织麦克风数据和参考数据
  // 格式：[mic0, ref0, mic1, ref1, mic2, ref2, ...]
  // 🎯 关键：参考信号需要延迟 kAecDelaySamples 来对齐麦克风采集到的回声
  {
    std::lock_guard<std::mutex> lock(ref_mutex_);
    for (int i = 0; i < actual_mic_samples; i++) {
      // 麦克风数据（转换为 int16）
      int32_t value = bit32_buffer[i] >> 12;
      dest[i * 2] = (value > INT16_MAX)    ? INT16_MAX
                    : (value < -INT16_MAX) ? -INT16_MAX
                                           : (int16_t)value;

      // 参考数据：从 write_pos 往回偏移 kAecDelaySamples
      // 这样参考信号就和麦克风采集到的回声时间对齐了
      size_t ref_pos = (ref_write_pos_ + kRefBufferSize - kAecDelaySamples -
                        actual_mic_samples + i) %
                       kRefBufferSize;
      dest[i * 2 + 1] = ref_buffer_[ref_pos];
    }
  }

  return actual_mic_samples * 2; // 返回总样本数（2 通道）
}

void NoAudioCodecSimplexAec::SetOutputSampleRate(int sample_rate) {
  if (sample_rate == output_sample_rate_) {
    return;
  }

  // I2S 必须先禁用才能重新配置时钟
  bool was_enabled = output_enabled_;
  if (was_enabled) {
    esp_err_t err = i2s_channel_disable(tx_handle_);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(err));
      return;
    }
  }

  i2s_std_clk_config_t clk_cfg = {
      .sample_rate_hz = (uint32_t)sample_rate,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
      .ext_clk_freq_hz = 0,
#endif
  };

  esp_err_t err = i2s_channel_reconfig_std_clock(tx_handle_, &clk_cfg);
  if (err == ESP_OK) {
    output_sample_rate_ = sample_rate;
    ESP_LOGI(TAG, "✅ Output sample rate changed to %d Hz",
             output_sample_rate_);
  } else {
    ESP_LOGE(TAG, "Failed to change output sample rate to %d Hz: %s",
             sample_rate, esp_err_to_name(err));
  }

  // 恢复之前的启用状态
  if (was_enabled) {
    err = i2s_channel_enable(tx_handle_);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to re-enable I2S channel: %s",
               esp_err_to_name(err));
    }
  }
}
