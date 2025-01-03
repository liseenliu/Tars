#include <esp_log.h>
#include "application.h"
#include "board.h"
#include "display/display.h"
#include "audio_codecs/audio_codec.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>

#define TAG "Application"

extern const char p3_err_reg_start[] asm("_binary_err_reg_p3_start");
extern const char p3_err_reg_end[] asm("_binary_err_reg_p3_end");
extern const char p3_err_pin_start[] asm("_binary_err_pin_p3_start");
extern const char p3_err_pin_end[] asm("_binary_err_pin_p3_end");
extern const char p3_err_wificonfig_start[] asm("_binary_err_wificonfig_p3_start");
extern const char p3_err_wificonfig_end[] asm("_binary_err_wificonfig_p3_end");

static const char* const STATE_STRINGS[] = {
    "unknown",
    "idle",
    "connecting",
    "listening",
    "speaking",
    "upgrading",
    "invalid_state"
};

Application::Application() : background_task_(4096 * 8)
{
    event_group_ = xEventGroupCreate();
}

Application::~Application()
{
    vEventGroupDelete(event_group_);
}

void Application::Start()
{
    auto &board = Board::GetInstance();
    // get led
    auto builtin_led = board.GetBuiltinLed();
    builtin_led->SetBlue();
    builtin_led->StartContinuousBlink(1000);
    // get display
    auto display = board.GetDisplay();

    auto codec = board.GetAudioCodec();
    opus_decode_sample_rate_ = codec->output_sample_rate();
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(opus_decode_sample_rate_, 1);
    opus_encoder_ = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_FRAME_DURATION_MS);

    // esp32s3 默认的rate 配置是16000 所以不会进这个判断，这个判断是为了给其他板子用的
    if (codec->input_sample_rate() != 16000)
    {
        input_resampler_.Configure(codec->input_sample_rate(), 16000);
        reference_resampler_.Configure(codec->input_sample_rate(), 16000);
    }
    codec->OnInputReady([this, codec]()
                        {
        BaseType_t higher_priority_task_woken = pdFALSE;
        xEventGroupSetBitsFromISR(event_group_, AUDIO_INPUT_READY_EVENT, &higher_priority_task_woken);
        return higher_priority_task_woken == pdTRUE; });

    codec->OnOutputReady([this]()
                         {
        BaseType_t higher_priority_task_woken = pdFALSE;
        xEventGroupSetBitsFromISR(event_group_, AUDIO_OUTPUT_READY_EVENT, &higher_priority_task_woken);
        return higher_priority_task_woken == pdTRUE; });

    codec->Start();

    /* Start the main loop */
    xTaskCreate([](void *arg)
                {
        Application* app = (Application*)arg;
        app->MainLoop();
        vTaskDelete(NULL); }, "main_loop", 4096 * 2, this, 2, nullptr);
    
    /* Wait for the network to be ready */
    board.StartNetwork();

    // Check for new firmware version or get the MQTT broker address
    xTaskCreate([](void *arg)
                {
        Application* app = (Application*)arg;
        app->CheckNewVersion();
        vTaskDelete(NULL); }, "check_new_version", 4096 * 2, this, 1, nullptr);

#if CONFIG_IDF_TARGET_ESP32S3
    // 初始化唤醒
    wake_word_detect_.Initialize(codec->input_channels(), codec->input_reference());
    wake_word_detect_.OnVadStateChange([this](bool speaking) {
        Schedule([this, speaking]() {
            auto builtin_led = Board::GetInstance().GetBuiltinLed();
            if (chat_state_ == kChatStateListening) {
                if (speaking) {
                    builtin_led->SetRed(HIGH_BRIGHTNESS);
                } else {
                    builtin_led->SetRed(LOW_BRIGHTNESS);
                }
                builtin_led->TurnOn();
            }
        });
    });

    wake_word_detect_.OnWakeWordDetected([this](const std::string& wake_word) {
        Schedule([this, &wake_word]() {
            if (chat_state_ == kChatStateIdle) {
                SetChatState(kChatStateConnecting);
                wake_word_detect_.EncodeWakeWordData();

                // if (!protocol_->OpenAudioChannel()) {
                //     ESP_LOGE(TAG, "Failed to open audio channel");
                //     SetChatState(kChatStateIdle);
                //     wake_word_detect_.StartDetection();
                //     return;
                // }
                
                // std::vector<uint8_t> opus;
                // // Encode and send the wake word data to the server
                // while (wake_word_detect_.GetWakeWordOpus(opus)) {
                //     protocol_->SendAudio(opus);
                // }
                // // Set the chat state to wake word detected
                // protocol_->SendWakeWordDetected(wake_word);
                ESP_LOGI(TAG, "Wake word detected: %s", wake_word.c_str());
                keep_listening_ = true;
                SetChatState(kChatStateListening);
            } else if (chat_state_ == kChatStateSpeaking) {
                // AbortSpeaking(kAbortReasonWakeWordDetected);
            }

            // Resume detection
            wake_word_detect_.StartDetection();
        });
    });

    wake_word_detect_.StartDetection();

    audio_processor_.Initialize(codec->input_channels(), codec->input_reference());
    audio_processor_.OnOutput([this](std::vector<int16_t>&& data) {
        background_task_.Schedule([this, data = std::move(data)]() mutable {
            opus_encoder_->Encode(std::move(data), [this](std::vector<uint8_t>&& opus) {
                Schedule([this, opus = std::move(opus)]() {
                    // protocol_->SendAudio(opus);
                });
            });
        });
    });

#endif
    display->SetStatus("待命");
    builtin_led->SetGreen();
    builtin_led->BlinkOnce();
    SetChatState(kChatStateIdle);
}

void Application::Schedule(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    main_tasks_.push_back(std::move(callback));
    xEventGroupSetBits(event_group_, SCHEDULE_EVENT);
}

void Application::SetChatState(ChatState state)
{
    if (chat_state_ == state) {
        return;
    }
    
    chat_state_ = state;
    ESP_LOGI(TAG, "STATE: %s", STATE_STRINGS[chat_state_]);
    // The state is changed, wait for all background tasks to finish
    background_task_.WaitForCompletion();
    auto display = Board::GetInstance().GetDisplay();
    auto builtin_led = Board::GetInstance().GetBuiltinLed();
    switch (state)
    {
    case kChatStateIdle:
        builtin_led->TurnOff();
        display->SetStatus("待命");
        display->SetEmotion("neutral");
#ifdef CONFIG_IDF_TARGET_ESP32S3
        audio_processor_.Stop();
#endif
        break;
    case kChatStateListening:
        builtin_led->SetRed();
        builtin_led->TurnOn();
        display->SetStatus("聆听中");
#if CONFIG_IDF_TARGET_ESP32S3
        audio_processor_.Start();
#endif
        break;
    case kChatStateSpeaking:
        builtin_led->SetGreen();
        builtin_led->TurnOn();
        display->SetStatus("说话中");
#if CONFIG_IDF_TARGET_ESP32S3
        audio_processor_.Stop();
#endif
        break;
    case kChatStateConnecting:
        builtin_led->SetBlue();
        builtin_led->TurnOn();
        display->SetStatus("连接中");
        break;
    case kChatStateUpgrading:
        display->SetStatus("升级中");
        builtin_led->SetGreen();
        builtin_led->StartContinuousBlink(100);
        break;
    default:
        ESP_LOGE(TAG, "Invalid chat state: %d", chat_state_);
        break;
    }
}

void Application::Alert(const std::string& title, const std::string& message) {
    ESP_LOGW(TAG, "Alert: %s, %s", title.c_str(), message.c_str());
    auto display = Board::GetInstance().GetDisplay();
    display->ShowNotification(message);

    if (message == "PIN is not ready") {
        PlayLocalFile(p3_err_pin_start, p3_err_pin_end - p3_err_pin_start);
    } else if (message == "Configuring WiFi") {
        PlayLocalFile(p3_err_wificonfig_start, p3_err_wificonfig_end - p3_err_wificonfig_start);
    } else if (message == "Registration denied") {
        PlayLocalFile(p3_err_reg_start, p3_err_reg_end - p3_err_reg_start);
    }
}


void Application::MainLoop()
{
    while (true)
    {
        auto bits = xEventGroupWaitBits(event_group_,
                                        SCHEDULE_EVENT | AUDIO_INPUT_READY_EVENT | AUDIO_OUTPUT_READY_EVENT,
                                        pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & AUDIO_INPUT_READY_EVENT)
        {
            InputAudio();
        }
        if (bits & AUDIO_OUTPUT_READY_EVENT)
        {
            OutputAudio();
        }
        if (bits & SCHEDULE_EVENT)
        {
            mutex_.lock();
            std::list<std::function<void()>> tasks = std::move(main_tasks_);
            mutex_.unlock();
            for (auto &task : tasks)
            {
                task();
            }
        }
    }
}

void Application::InputAudio()
{
    auto codec = Board::GetInstance().GetAudioCodec();
    std::vector<int16_t> data;

    if (!codec->InputData(data))
    {
        return;
    }

    if (codec->input_sample_rate() != 16000) {
        if (codec->input_channels() == 2) {
            auto mic_channel = std::vector<int16_t>(data.size() / 2);
            auto reference_channel = std::vector<int16_t>(data.size() / 2);
            for (size_t i = 0, j = 0; i < mic_channel.size(); ++i, j += 2) {
                mic_channel[i] = data[j];
                reference_channel[i] = data[j + 1];
            }
            auto resampled_mic = std::vector<int16_t>(input_resampler_.GetOutputSamples(mic_channel.size()));
            auto resampled_reference = std::vector<int16_t>(reference_resampler_.GetOutputSamples(reference_channel.size()));
            input_resampler_.Process(mic_channel.data(), mic_channel.size(), resampled_mic.data());
            reference_resampler_.Process(reference_channel.data(), reference_channel.size(), resampled_reference.data());
            data.resize(resampled_mic.size() + resampled_reference.size());
            for (size_t i = 0, j = 0; i < resampled_mic.size(); ++i, j += 2) {
                data[j] = resampled_mic[i];
                data[j + 1] = resampled_reference[i];
            }
        } else {
            auto resampled = std::vector<int16_t>(input_resampler_.GetOutputSamples(data.size()));
            input_resampler_.Process(data.data(), data.size(), resampled.data());
            data = std::move(resampled);
        }
    }

    // ESP_LOGI(TAG, "Input %d samples", data.size());

#if CONFIG_IDF_TARGET_ESP32S3
    if (audio_processor_.IsRunning())
    {
        audio_processor_.Input(data);
    }

    if (wake_word_detect_.IsDetectionRunning()) {
        wake_word_detect_.Feed(data);
    }
#endif
}

void Application::OutputAudio()
{
    auto now = std::chrono::steady_clock::now();
    auto codec = Board::GetInstance().GetAudioCodec();
    const int max_silence_seconds = 10;

    std::unique_lock<std::mutex> lock(mutex_);
    if (audio_decode_queue_.empty()) {
        // Disable the output if there is no audio data for a long time
        if (chat_state_ == kChatStateIdle) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_output_time_).count();
            if (duration > max_silence_seconds) {
                codec->EnableOutput(false);
            }
        }
        return;
    }

    if (chat_state_ == kChatStateListening) {
        audio_decode_queue_.clear();
        return;
    }

    last_output_time_ = now;
    auto opus = std::move(audio_decode_queue_.front());
    audio_decode_queue_.pop_front();
    lock.unlock();

    background_task_.Schedule([this, codec, opus = std::move(opus)]() mutable {
        if (aborted_) {
            return;
        }

        std::vector<int16_t> pcm;
        if (!opus_decoder_->Decode(std::move(opus), pcm)) {
            return;
        }

        // Resample if the sample rate is different
        if (opus_decode_sample_rate_ != codec->output_sample_rate()) {
            int target_size = output_resampler_.GetOutputSamples(pcm.size());
            std::vector<int16_t> resampled(target_size);
            output_resampler_.Process(pcm.data(), pcm.size(), resampled.data());
            pcm = std::move(resampled);
        }
        
        codec->OutputData(pcm);
    });

}

void Application::ResetDecoder()
{
}

void Application::SetDecodeSampleRate(int sample_rate)
{
    if (opus_decode_sample_rate_ == sample_rate) {
        return;
    }

    opus_decode_sample_rate_ = sample_rate;
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(opus_decode_sample_rate_, 1);

    auto codec = Board::GetInstance().GetAudioCodec();
    if (opus_decode_sample_rate_ != codec->output_sample_rate()) {
        ESP_LOGI(TAG, "Resampling audio from %d to %d", opus_decode_sample_rate_, codec->output_sample_rate());
        output_resampler_.Configure(opus_decode_sample_rate_, codec->output_sample_rate());
    }
}

void Application::CheckNewVersion()
{
}

void Application::PlayLocalFile(const char* data, size_t size)
{
    ESP_LOGI(TAG, "PlayLocalFile: %zu bytes", size);
    SetDecodeSampleRate(16000);
    for (const char* p = data; p < data + size; ) {
        auto p3 = (BinaryProtocol3*)p;
        p += sizeof(BinaryProtocol3);

        auto payload_size = ntohs(p3->payload_size);
        std::vector<uint8_t> opus;
        opus.resize(payload_size);
        memcpy(opus.data(), p3->payload, payload_size);
        p += payload_size;

        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.emplace_back(std::move(opus));
    }
}
