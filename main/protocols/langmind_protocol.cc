#include "langmind_protocol.h"

#include "application.h"
#include "board.h"
#include "settings.h"
#include "system_info.h"

#include <cJSON.h>
#include <esp_audio_dec.h>
#include <esp_log.h>
#include "audio_service.h"
#include <esp_opus_dec.h>
#include <esp_timer.h>

#ifndef OPUS_DEC_CFG
#define OPUS_DEC_CFG(_sample_rate, _frame_duration_ms)                                     \
    (esp_opus_dec_cfg_t) {                                                                 \
        .sample_rate = (uint32_t)(_sample_rate), .channel = ESP_AUDIO_MONO,                \
        .frame_duration =                                                                  \
            (esp_opus_dec_frame_duration_t)AS_OPUS_GET_FRAME_DRU_ENUM(_frame_duration_ms), \
        .self_delimited = false,                                                           \
    }
#endif

#include <algorithm>
#include <cstring>

namespace {
const char* TAG = "LangMind";

constexpr EventBits_t kEventStartProcessing = (1 << 0);
constexpr EventBits_t kEventAbort = (1 << 1);
constexpr EventBits_t kEventStopWorker = (1 << 2);

constexpr int kHttpTimeoutMs = 15000;
constexpr int kChatHttpTimeoutMs = 35000;
constexpr size_t kPcmSamplesPer60ms = 960;  // 16 kHz * 60 ms / 1000
constexpr size_t kPcmBytesPer60ms = kPcmSamplesPer60ms * sizeof(int16_t);

struct __attribute__((packed)) WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_size = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;     // PCM
    uint16_t num_channels = 1;     // mono
    uint32_t sample_rate = 16000;  // 16kHz
    uint32_t byte_rate = 32000;    // 16000 * 1 * 2
    uint16_t block_align = 2;      // 1 * 2
    uint16_t bits_per_sample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size = 0;
};

struct ParsedWav {
    bool valid = false;
    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    size_t data_offset = 0;
    uint32_t data_size = 0;
    std::string error;
};

ParsedWav ParseWavHeader(const uint8_t* data, size_t size) {
    ParsedWav result;
    if (size < 44) {
        result.error = "WAV header too short (< 44 bytes)";
        return result;
    }
    if (std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0) {
        result.error = "Not a valid RIFF/WAVE stream";
        return result;
    }

    size_t offset = 12;
    bool found_fmt = false;
    bool found_data = false;

    while (offset + 8 <= size) {
        char chunk_id[5] = {0};
        std::memcpy(chunk_id, data + offset, 4);
        uint32_t chunk_size = 0;
        std::memcpy(&chunk_size, data + offset + 4, 4);
        offset += 8;

        if (std::strcmp(chunk_id, "fmt ") == 0) {
            if (offset + 16 > size) {
                result.error = "Truncated fmt chunk in WAV";
                return result;
            }
            std::memcpy(&result.audio_format, data + offset, 2);
            std::memcpy(&result.channels, data + offset + 2, 2);
            std::memcpy(&result.sample_rate, data + offset + 4, 4);
            std::memcpy(&result.bits_per_sample, data + offset + 14, 2);
            found_fmt = true;
            offset += chunk_size;
        } else if (std::strcmp(chunk_id, "data") == 0) {
            result.data_offset = offset;
            result.data_size = chunk_size;
            found_data = true;
            break;
        } else {
            // Skip non-data chunk (JUNK, LIST, etc.)
            offset += chunk_size;
        }
    }

    if (!found_fmt || !found_data) {
        result.error = "Missing fmt or data chunk in WAV";
        return result;
    }

    if (result.audio_format != 1) {
        result.error = "Unsupported WAV audio format: " + std::to_string(result.audio_format) +
                       " (only PCM=1 is supported)";
        return result;
    }
    if (result.bits_per_sample != 16) {
        result.error = "Unsupported WAV bit depth: " + std::to_string(result.bits_per_sample) +
                       " (only 16-bit PCM is supported)";
        return result;
    }
    if (result.channels != 1 && result.channels != 2) {
        result.error = "Unsupported WAV channel count: " + std::to_string(result.channels) +
                       " (only mono or stereo supported)";
        return result;
    }
    if (result.sample_rate < 8000 || result.sample_rate > 48000) {
        result.error = "Unsupported WAV sample rate: " + std::to_string(result.sample_rate);
        return result;
    }

    result.valid = true;
    return result;
}

}  // namespace

LangMindProtocol::LangMindProtocol() {
    event_group_ = xEventGroupCreate();
}

LangMindProtocol::~LangMindProtocol() {
    CloseAudioChannel(false);
    if (worker_task_handle_ != nullptr) {
        xEventGroupSetBits(event_group_, kEventStopWorker);
        for (int i = 0; i < 50 && worker_running_.load(); ++i) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        worker_task_handle_ = nullptr;
    }
    if (event_group_ != nullptr) {
        vEventGroupDelete(event_group_);
        event_group_ = nullptr;
    }
}

bool LangMindProtocol::Start() {
    return true;
}

bool LangMindProtocol::OpenAudioChannel() {
    Settings settings("langmind", false);
    api_key_ = settings.GetString("api_key");
#ifdef CONFIG_LANGMIND_API_KEY
    if (api_key_.empty()) {
        api_key_ = CONFIG_LANGMIND_API_KEY;
    }
#endif
    agent_id_ = settings.GetString("agent_id");
#ifdef CONFIG_LANGMIND_AGENT_ID
    if (agent_id_.empty()) {
        agent_id_ = CONFIG_LANGMIND_AGENT_ID;
    }
#endif
    if (agent_id_.empty()) {
        agent_id_ = "e2fe0242-6e05-4460-b846-1fab2f8df2e9";
    }

    base_url_ = settings.GetString("base_url");
#ifdef CONFIG_LANGMIND_BASE_URL
    if (base_url_.empty()) {
        base_url_ = CONFIG_LANGMIND_BASE_URL;
    }
#endif
    if (base_url_.empty()) {
        base_url_ = "https://langmind.in.th";
    }
    while (base_url_.length() > 0 && base_url_.back() == '/') {
        base_url_.pop_back();
    }

    int sp = settings.GetInt("voice_speed");
    if (sp > 0) {
        voice_speed_ = static_cast<float>(sp) / 100.0f;
    }
    voice_override_ = settings.GetString("voice");

    if (api_key_.empty()) {
        ESP_LOGE(TAG, "LangMind API Key is not set in NVS namespace 'langmind', key 'api_key'");
        SetError("API Key missing", "Please provision 'api_key' in NVS namespace 'langmind'");
        return false;
    }

    error_occurred_ = false;
    abort_requested_ = false;

    if (worker_task_handle_ == nullptr) {
        worker_running_ = true;
        BaseType_t ret = xTaskCreate(WorkerEntry, "lm_worker", 8192, this, 3, &worker_task_handle_);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create LangMind worker task");
            worker_running_ = false;
            return false;
        }
    }

    is_channel_opened_ = true;
    if (on_audio_channel_opened_ != nullptr) {
        on_audio_channel_opened_();
    }
    if (on_connected_ != nullptr) {
        on_connected_();
    }

    if (!settings_checked_.exchange(true)) {
        CheckVoiceSettings();
    }

    return true;
}

void LangMindProtocol::CloseAudioChannel(bool send_goodbye) {
    if (send_goodbye) {
        conversation_id_.clear();
    }
    is_recording_ = false;
    is_channel_opened_ = false;
    abort_requested_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        recorded_packets_.clear();
    }

    if (event_group_ != nullptr) {
        xEventGroupSetBits(event_group_, kEventAbort);
    }

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }
    if (on_disconnected_ != nullptr) {
        on_disconnected_();
    }
}

bool LangMindProtocol::IsAudioChannelOpened() const {
    return is_channel_opened_.load() && !error_occurred_;
}

void LangMindProtocol::SendStartListening(ListeningMode mode) {
    listening_mode_ = mode;
    abort_requested_ = false;
    voice_started_ = false;
    last_voice_detected_time_us_ = 0;
    recording_start_time_us_ = esp_timer_get_time();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        recorded_packets_.clear();
    }

    is_recording_ = true;
    ESP_LOGI(TAG, "Start listening (mode: %d)", static_cast<int>(mode));
}

void LangMindProtocol::SendStopListening() {
    if (is_recording_.load()) {
        ESP_LOGI(TAG, "Stop listening requested manually");
        TriggerProcessing();
    }
}

void LangMindProtocol::SendAbortSpeaking(AbortReason reason) {
    (void)reason;
    ESP_LOGI(TAG, "Abort speaking requested");
    abort_requested_ = true;
    is_recording_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        recorded_packets_.clear();
    }
    Application::GetInstance().GetAudioService().ResetDecoder();
    if (event_group_ != nullptr) {
        xEventGroupSetBits(event_group_, kEventAbort);
    }
}

void LangMindProtocol::SendWakeWordDetected(const std::string& wake_word) {
    ESP_LOGI(TAG, "Wake word detected: %s", wake_word.c_str());
    SendStartListening(listening_mode_);
}

void LangMindProtocol::SendMcpMessage(const std::string& message) {
    ESP_LOGI(TAG, "MCP message (not applicable for direct LangMind): %s", message.c_str());
}

void LangMindProtocol::ResetConversation() {
    conversation_id_.clear();
    ESP_LOGI(TAG, "Conversation ID cleared, starting new conversation on next utterance");
}

bool LangMindProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!is_recording_.load() || abort_requested_.load()) {
        return false;
    }

    int64_t now_us = esp_timer_get_time();
    bool voice_now = Application::GetInstance().IsVoiceDetected();

    if (voice_now) {
        if (!voice_started_) {
            voice_started_ = true;
            ESP_LOGI(TAG, "Speech start detected by VAD");
        }
        last_voice_detected_time_us_ = now_us;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (recorded_packets_.size() >= kMaxRecordedPackets) {
            ESP_LOGW(TAG, "Audio buffer reached maximum (%u packets, ~%u sec), triggering processing",
                     static_cast<unsigned int>(kMaxRecordedPackets),
                     static_cast<unsigned int>(kMaxRecordedPackets * 60 / 1000));
            TriggerProcessing();
            return false;
        }
        recorded_packets_.push_back(std::move(packet));
    }

    // Auto-stop detection: if voice started and user has been silent for threshold
    if (listening_mode_ == kListeningModeAutoStop && voice_started_) {
        if (last_voice_detected_time_us_ > 0 &&
            (now_us - last_voice_detected_time_us_) >= kSilenceThresholdUs) {
            ESP_LOGI(TAG, "Speech end detected by silence (%lld ms), triggering processing",
                     (now_us - last_voice_detected_time_us_) / 1000);
            TriggerProcessing();
            return false;
        }
    }

    return true;
}

void LangMindProtocol::TriggerProcessing() {
    is_recording_ = false;
    if (event_group_ != nullptr) {
        xEventGroupSetBits(event_group_, kEventStartProcessing);
    }
}

bool LangMindProtocol::SendText(const std::string& text) {
    (void)text;
    return true;
}

void LangMindProtocol::WorkerEntry(void* arg) {
    auto* self = static_cast<LangMindProtocol*>(arg);
    self->WorkerLoop();
    self->worker_running_ = false;
    vTaskDelete(nullptr);
}

void LangMindProtocol::WorkerLoop() {
    while (worker_running_.load()) {
        EventBits_t bits = xEventGroupWaitBits(
            event_group_, kEventStartProcessing | kEventStopWorker, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & kEventStopWorker) {
            break;
        }

        if (bits & kEventStartProcessing) {
            if (abort_requested_.load()) {
                abort_requested_ = false;
                continue;
            }

            std::vector<std::unique_ptr<AudioStreamPacket>> packets;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                packets = std::move(recorded_packets_);
            }

            if (packets.size() < 4) {
                ESP_LOGI(TAG, "Audio too short (%u packets < 240ms), ignoring",
                         static_cast<unsigned int>(packets.size()));
                continue;
            }

            is_processing_ = true;
            HandleSpeechProcessing(std::move(packets));
            is_processing_ = false;
        }
    }
}

void LangMindProtocol::HandleSpeechProcessing(std::vector<std::unique_ptr<AudioStreamPacket>> packets) {
    ESP_LOGI(TAG, "Processing %u audio packets...", static_cast<unsigned int>(packets.size()));

    // 1. Transcribe
    std::string text = TranscribeAudio(packets);
    if (abort_requested_.load() || error_occurred_) {
        return;
    }

    if (text.empty()) {
        ESP_LOGI(TAG, "No speech recognized");
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "stt");
        cJSON_AddStringToObject(root, "text", "");
        if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);

        // Transition back to idle
        Application::GetInstance().Schedule([]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateListening) {
                app.SetDeviceState(kDeviceStateIdle);
            }
        });
        return;
    }

    // Display user speech
    {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "stt");
        cJSON_AddStringToObject(root, "text", text.c_str());
        if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);
    }

    // Display thinking emotion
    {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "llm");
        cJSON_AddStringToObject(root, "emotion", "thinking");
        if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);
    }

    // 2. Chat with agent
    std::string answer = ChatMessage(text);
    if (abort_requested_.load() || error_occurred_) {
        return;
    }

    if (answer.empty()) {
        ESP_LOGW(TAG, "Chat returned empty answer");
        Application::GetInstance().Schedule([]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateListening ||
                app.GetDeviceState() == kDeviceStateSpeaking) {
                app.SetDeviceState(kDeviceStateIdle);
            }
        });
        return;
    }

    // Display happy/talking emotion
    {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "llm");
        cJSON_AddStringToObject(root, "emotion", "happy");
        if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);
    }

    // 3. Synthesize and Play
    SynthesizeAndPlay(answer);
}

std::string LangMindProtocol::TranscribeAudio(
    const std::vector<std::unique_ptr<AudioStreamPacket>>& packets) {
    if (packets.empty()) {
        return "";
    }

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        ESP_LOGE(TAG, "Network interface is null");
        return "";
    }

    auto http = network->CreateHttp(0);
    if (!http) {
        ESP_LOGE(TAG, "Failed to create HTTP client for STT");
        return "";
    }

    std::string boundary = "----LangMindEsp32Boundary" + std::to_string(esp_timer_get_time());
    std::string url = base_url_ + "/api/v1/agents/" + agent_id_ + "/voice/transcribe";

    const size_t total_pcm_bytes = packets.size() * kPcmBytesPer60ms;
    const size_t total_wav_bytes = sizeof(WavHeader) + total_pcm_bytes;

    std::string part_header;
    part_header += "--" + boundary + "\r\n";
    part_header += "Content-Disposition: form-data; name=\"audio\"; filename=\"audio.wav\"\r\n";
    part_header += "Content-Type: audio/wav\r\n\r\n";

    std::string part_footer = "\r\n--" + boundary + "--\r\n";
    const size_t content_length = part_header.size() + total_wav_bytes + part_footer.size();

    http->SetTimeout(kHttpTimeoutMs);
    http->SetHeader("Authorization", "Bearer " + api_key_);
    http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http->SetHeader("Content-Length", std::to_string(content_length));

    ESP_LOGI(TAG, "Calling LangMind STT: %s (%u frames, %u bytes WAV)", url.c_str(),
             static_cast<unsigned int>(packets.size()), static_cast<unsigned int>(total_wav_bytes));

    if (auto opened = http->Open("POST", url); !opened) {
        ESP_LOGE(TAG, "Failed to open STT HTTP connection: %s", opened.error().ToString().c_str());
        SetError("STT connection failed", opened.error().ToString());
        return "";
    }

    // Write part header
    if (auto res = http->Write(part_header.data(), part_header.size()); !res) {
        ESP_LOGE(TAG, "Failed to write part header: %s", res.error().ToString().c_str());
        http->Close();
        return "";
    }

    // Write WAV header
    WavHeader wav_hdr;
    wav_hdr.riff_size = static_cast<uint32_t>(36 + total_pcm_bytes);
    wav_hdr.data_size = static_cast<uint32_t>(total_pcm_bytes);
    if (auto res = http->Write(reinterpret_cast<const char*>(&wav_hdr), sizeof(wav_hdr)); !res) {
        ESP_LOGE(TAG, "Failed to write WAV header: %s", res.error().ToString().c_str());
        http->Close();
        return "";
    }

    // Initialize temporary Opus decoder to decode buffered frames to 16kHz PCM
    esp_opus_dec_cfg_t dec_cfg = OPUS_DEC_CFG(16000, 60);
    void* opus_dec = nullptr;
    auto dec_err = esp_opus_dec_open(&dec_cfg, sizeof(dec_cfg), &opus_dec);
    if (dec_err != ESP_AUDIO_ERR_OK || opus_dec == nullptr) {
        ESP_LOGE(TAG, "Failed to create temporary Opus decoder: %d", dec_err);
        http->Close();
        return "";
    }

    std::vector<int16_t> pcm_chunk(kPcmSamplesPer60ms);
    bool write_error = false;

    for (const auto& packet : packets) {
        if (abort_requested_.load()) {
            write_error = true;
            break;
        }

        // If payload is already raw 16kHz PCM (size matches), send directly
        if (packet->payload.size() == kPcmBytesPer60ms) {
            if (auto res = http->Write(reinterpret_cast<const char*>(packet->payload.data()),
                                       packet->payload.size());
                !res) {
                write_error = true;
                break;
            }
            continue;
        }

        // Decode Opus packet to PCM
        esp_audio_dec_in_raw_t raw = {
            .buffer = packet->payload.data(),
            .len = static_cast<uint32_t>(packet->payload.size()),
            .consumed = 0,
            .frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE,
        };
        esp_audio_dec_out_frame_t out = {
            .buffer = reinterpret_cast<uint8_t*>(pcm_chunk.data()),
            .len = static_cast<uint32_t>(kPcmBytesPer60ms),
            .decoded_size = 0,
        };
        esp_audio_dec_info_t dec_info = {};

        auto ret = esp_opus_dec_decode(opus_dec, &raw, &out, &dec_info);
        if (ret == ESP_AUDIO_ERR_OK && out.decoded_size > 0) {
            if (auto res = http->Write(reinterpret_cast<const char*>(pcm_chunk.data()),
                                       out.decoded_size);
                !res) {
                write_error = true;
                break;
            }
        } else {
            // Write silence for lost or corrupted frame
            std::memset(pcm_chunk.data(), 0, kPcmBytesPer60ms);
            if (auto res = http->Write(reinterpret_cast<const char*>(pcm_chunk.data()),
                                       kPcmBytesPer60ms);
                !res) {
                write_error = true;
                break;
            }
        }
    }

    esp_opus_dec_close(opus_dec);

    if (write_error) {
        http->Close();
        return "";
    }

    // Write part footer
    if (auto res = http->Write(part_footer.data(), part_footer.size()); !res) {
        http->Close();
        return "";
    }
    http->Write("", 0);  // End of write stream

    auto status = http->GetStatusCode();
    if (!status) {
        ESP_LOGE(TAG, "Failed to get STT status code: %s", status.error().ToString().c_str());
        http->Close();
        return "";
    }

    std::string response = http->ReadAll();
    http->Close();

    if (*status == 422) {
        ESP_LOGE(TAG,
                 "STT Error [422 VOICE_AUDIO_FORMAT_UNSUPPORTED]: Sent 16kHz mono WAV (%u bytes). "
                 "Response: %s",
                 static_cast<unsigned int>(total_wav_bytes), response.c_str());
        SetError("STT format unsupported (422)", "Sent WAV 16kHz mono, server rejected format");
        return "";
    }

    if (*status == 403) {
        ESP_LOGE(TAG,
                 "STT Error [403 VOICE_DISABLED]: Voice is disabled for agent %s. Please enable "
                 "voice chat and publish settings.",
                 agent_id_.c_str());
        SetError("Voice disabled (403)", "Enable voice in LangMind agent settings and publish");
        return "";
    }

    if (*status != 200) {
        ESP_LOGE(TAG, "STT Error [%d]: %s", *status, response.c_str());
        SetError("STT error (" + std::to_string(*status) + ")");
        return "";
    }

    // Parse JSON
    cJSON* root = cJSON_Parse(response.c_str());
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse STT response JSON: %s", response.c_str());
        return "";
    }

    std::string result_text;
    cJSON* text_obj = cJSON_GetObjectItem(root, "text");
    if (!cJSON_IsString(text_obj)) {
        cJSON* data_obj = cJSON_GetObjectItem(root, "data");
        if (cJSON_IsObject(data_obj)) {
            text_obj = cJSON_GetObjectItem(data_obj, "text");
        }
    }

    if (cJSON_IsString(text_obj) && text_obj->valuestring != nullptr) {
        result_text = text_obj->valuestring;
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "STT transcribed text: '%s'", result_text.c_str());
    return result_text;
}

std::string LangMindProtocol::ChatMessage(const std::string& query) {
    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        return "";
    }

    auto http = network->CreateHttp(0);
    if (!http) {
        return "";
    }

    std::string url = base_url_ + "/api/chat/message/blocking";

    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "agent_id", agent_id_.c_str());
    cJSON_AddStringToObject(req, "query", query.c_str());
    if (!conversation_id_.empty()) {
        cJSON_AddStringToObject(req, "conversation_id", conversation_id_.c_str());
    } else {
        cJSON_AddNullToObject(req, "conversation_id");
    }
    cJSON_AddStringToObject(req, "input_mode", "voice");
    cJSON_AddArrayToObject(req, "attachment_ids");

    char* json_payload = cJSON_PrintUnformatted(req);
    std::string body(json_payload != nullptr ? json_payload : "{}");
    cJSON_free(json_payload);
    cJSON_Delete(req);

    http->SetTimeout(kChatHttpTimeoutMs);
    http->SetHeader("Authorization", "Bearer " + api_key_);
    http->SetHeader("Content-Type", "application/json");
    http->SetContent(std::move(body));

    ESP_LOGI(TAG, "Calling LangMind Chat: %s | query: '%s'", url.c_str(), query.c_str());

    if (auto opened = http->Open("POST", url); !opened) {
        ESP_LOGE(TAG, "Failed to open Chat connection: %s", opened.error().ToString().c_str());
        SetError("Chat connection failed");
        return "";
    }

    auto status = http->GetStatusCode();
    if (!status) {
        ESP_LOGE(TAG, "Failed to get Chat status code");
        http->Close();
        return "";
    }

    std::string response = http->ReadAll();
    http->Close();

    if (*status != 200) {
        ESP_LOGE(TAG, "Chat Error [%d]: %s", *status, response.c_str());
        SetError("Chat error (" + std::to_string(*status) + ")");
        return "";
    }

    cJSON* root = cJSON_Parse(response.c_str());
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse Chat response JSON: %s", response.c_str());
        return "";
    }

    cJSON* target = root;
    cJSON* data_obj = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsObject(data_obj)) {
        target = data_obj;
    }

    std::string answer;
    for (const char* key : {"answer", "message", "content", "text"}) {
        cJSON* item = cJSON_GetObjectItem(target, key);
        if (cJSON_IsString(item) && item->valuestring != nullptr && item->valuestring[0] != '\0') {
            answer = item->valuestring;
            break;
        }
    }

    cJSON* conv_item = cJSON_GetObjectItem(target, "conversation_id");
    if (!cJSON_IsString(conv_item)) {
        conv_item = cJSON_GetObjectItem(root, "conversation_id");
    }
    if (cJSON_IsString(conv_item) && conv_item->valuestring != nullptr) {
        conversation_id_ = conv_item->valuestring;
        ESP_LOGI(TAG, "Updated conversation_id: %s", conversation_id_.c_str());
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Chat answer: '%s'", answer.c_str());
    return answer;
}

std::vector<std::string> LangMindProtocol::SplitTextIntoChunks(const std::string& text,
                                                               size_t max_bytes) {
    std::vector<std::string> chunks;
    if (text.empty()) {
        return chunks;
    }
    if (text.size() <= max_bytes) {
        chunks.push_back(text);
        return chunks;
    }

    size_t start = 0;
    while (start < text.size()) {
        if (text.size() - start <= max_bytes) {
            chunks.push_back(text.substr(start));
            break;
        }

        size_t end = start + max_bytes;
        // Search backwards for a natural sentence or whitespace delimiter
        size_t split_pos = std::string::npos;
        for (size_t i = end; i > start + (max_bytes / 2); --i) {
            char c = text[i - 1];
            if (c == '\n' || c == '.' || c == '?' || c == '!' || c == ' ') {
                split_pos = i;
                break;
            }
        }

        if (split_pos == std::string::npos) {
            split_pos = end;
        }

        // Ensure we do not split in the middle of a multi-byte UTF-8 sequence
        while (split_pos > start && (static_cast<uint8_t>(text[split_pos]) & 0xC0) == 0x80) {
            --split_pos;
        }

        if (split_pos == start) {
            // Cannot split cleanly backwards, find the next UTF-8 boundary forward
            split_pos = end;
            while (split_pos < text.size() &&
                   (static_cast<uint8_t>(text[split_pos]) & 0xC0) == 0x80) {
                ++split_pos;
            }
        }

        chunks.push_back(text.substr(start, split_pos - start));
        start = split_pos;
    }

    return chunks;
}

bool LangMindProtocol::SynthesizeAndPlay(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    auto chunks = SplitTextIntoChunks(text, 3800);
    ESP_LOGI(TAG, "Synthesizing text (%u chars) across %u chunk(s)",
             static_cast<unsigned int>(text.length()), static_cast<unsigned int>(chunks.size()));

    for (size_t i = 0; i < chunks.size(); ++i) {
        if (abort_requested_.load()) {
            break;
        }

        const auto& chunk = chunks[i];
        auto network = Board::GetInstance().GetNetwork();
        if (network == nullptr) {
            return false;
        }

        auto http = network->CreateHttp(0);
        if (!http) {
            return false;
        }

        std::string url = base_url_ + "/api/v1/agents/" + agent_id_ + "/voice/speech";

        cJSON* req = cJSON_CreateObject();
        cJSON_AddStringToObject(req, "text", chunk.c_str());
        cJSON_AddStringToObject(req, "format", "wav");
        if (voice_speed_ > 0.0f && voice_speed_ != 1.0f) {
            cJSON_AddNumberToObject(req, "speed", voice_speed_);
        }
        if (!voice_override_.empty()) {
            cJSON_AddStringToObject(req, "voice", voice_override_.c_str());
        }

        char* json_payload = cJSON_PrintUnformatted(req);
        std::string body(json_payload != nullptr ? json_payload : "{}");
        cJSON_free(json_payload);
        cJSON_Delete(req);

        http->SetTimeout(kHttpTimeoutMs);
        http->SetHeader("Authorization", "Bearer " + api_key_);
        http->SetHeader("Content-Type", "application/json");
        http->SetHeader("Accept", "audio/wav, audio/*");
        http->SetContent(std::move(body));

        ESP_LOGI(TAG, "Calling LangMind TTS: %s (chunk %u/%u)", url.c_str(),
                 static_cast<unsigned int>(i + 1), static_cast<unsigned int>(chunks.size()));

        if (auto opened = http->Open("POST", url); !opened) {
            ESP_LOGE(TAG, "Failed to open TTS connection: %s", opened.error().ToString().c_str());
            SetError("TTS connection failed");
            return false;
        }

        auto status = http->GetStatusCode();
        if (!status || *status != 200) {
            int code = status ? *status : -1;
            std::string err_body = http->ReadAll();
            http->Close();
            ESP_LOGE(TAG, "TTS Error [%d]: %s", code, err_body.c_str());
            if (code == 403) {
                SetError("Voice disabled (403)", "Enable voice in LangMind agent settings and publish");
            } else {
                SetError("TTS error (" + std::to_string(code) + ")");
            }
            return false;
        }

        // Notify Application that assistant started speaking
        if (i == 0) {
            cJSON* root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "tts");
            cJSON_AddStringToObject(root, "state", "start");
            if (on_incoming_json_ != nullptr) {
                on_incoming_json_(root);
            }
            cJSON_Delete(root);
        }
        {
            cJSON* root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "tts");
            cJSON_AddStringToObject(root, "state", "sentence_start");
            cJSON_AddStringToObject(root, "text", chunk.c_str());
            if (on_incoming_json_ != nullptr) {
                on_incoming_json_(root);
            }
            cJSON_Delete(root);
        }

        // Read WAV header from HTTP response stream
        std::vector<uint8_t> header_buf(256);
        size_t header_bytes_read = 0;
        ParsedWav wav_info;

        while (!wav_info.valid && header_bytes_read < header_buf.size() && !abort_requested_.load()) {
            auto read_res = http->Read(reinterpret_cast<char*>(header_buf.data() + header_bytes_read),
                                       header_buf.size() - header_bytes_read);
            if (!read_res || *read_res == 0) {
                break;
            }
            header_bytes_read += *read_res;
            wav_info = ParseWavHeader(header_buf.data(), header_bytes_read);
        }

        if (!wav_info.valid) {
            ESP_LOGE(TAG, "TTS returned invalid/unsupported WAV audio: %s", wav_info.error.c_str());
            http->Close();
            SetError("Invalid TTS audio format", wav_info.error);
            return false;
        }

        ESP_LOGI(TAG, "Playing TTS WAV: %lu Hz, %u ch, %u bit (offset %u, data %lu bytes)",
                 static_cast<unsigned long>(wav_info.sample_rate), wav_info.channels,
                 wav_info.bits_per_sample, static_cast<unsigned int>(wav_info.data_offset),
                 static_cast<unsigned long>(wav_info.data_size));

        auto& audio_service = Application::GetInstance().GetAudioService();

        size_t leftover = 0;
        std::array<char, 1920> stream_buf;

        // Feed any initial PCM data already read in header_buf
        if (header_bytes_read > wav_info.data_offset) {
            const size_t initial_pcm_bytes = header_bytes_read - wav_info.data_offset;
            const size_t frame_bytes = wav_info.channels * sizeof(int16_t);
            const size_t usable_bytes = (initial_pcm_bytes / frame_bytes) * frame_bytes;
            const size_t total_samples = usable_bytes / sizeof(int16_t);
            const int16_t* pcm_src = reinterpret_cast<const int16_t*>(header_buf.data() + wav_info.data_offset);

            if (total_samples > 0) {
                std::vector<int16_t> pcm;
                if (wav_info.channels == 2) {
                    pcm.resize(total_samples / 2);
                    for (size_t s = 0; s < pcm.size(); ++s) {
                        pcm[s] = pcm_src[s * 2];
                    }
                } else {
                    pcm.assign(pcm_src, pcm_src + total_samples);
                }
                audio_service.PushPcmToPlaybackQueue(std::move(pcm), wav_info.sample_rate, true);
            }

            leftover = initial_pcm_bytes - usable_bytes;
            if (leftover > 0) {
                std::memcpy(stream_buf.data(), header_buf.data() + wav_info.data_offset + usable_bytes, leftover);
            }
        }

        // Stream remaining PCM from HTTP response straight to speaker
        while (!abort_requested_.load()) {
            auto read_res = http->Read(stream_buf.data() + leftover, stream_buf.size() - leftover);
            if (!read_res || *read_res == 0) {
                break;
            }

            const size_t total_bytes = leftover + *read_res;
            const size_t frame_bytes = wav_info.channels * sizeof(int16_t);
            const size_t usable_bytes = (total_bytes / frame_bytes) * frame_bytes;
            const size_t total_samples = usable_bytes / sizeof(int16_t);
            const int16_t* pcm_src = reinterpret_cast<const int16_t*>(stream_buf.data());

            if (total_samples > 0) {
                std::vector<int16_t> pcm;
                if (wav_info.channels == 2) {
                    pcm.resize(total_samples / 2);
                    for (size_t s = 0; s < pcm.size(); ++s) {
                        pcm[s] = pcm_src[s * 2];
                    }
                } else {
                    pcm.assign(pcm_src, pcm_src + total_samples);
                }

                if (!audio_service.PushPcmToPlaybackQueue(std::move(pcm), wav_info.sample_rate, true)) {
                    // Playback queue rejected or aborted
                    break;
                }
            }

            leftover = total_bytes - usable_bytes;
            if (leftover > 0) {
                std::memmove(stream_buf.data(), stream_buf.data() + usable_bytes, leftover);
            }
        }

        http->Close();
    }

    // Wait until playback queue has completely drained through speaker
    auto& audio_service = Application::GetInstance().GetAudioService();
    for (int t = 0; t < 200 && !audio_service.IsPlaybackIdle() && !abort_requested_.load(); ++t) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Notify Application that TTS has finished
    {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "tts");
        cJSON_AddStringToObject(root, "state", "stop");
        if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);
    }

    return true;
}

void LangMindProtocol::CheckVoiceSettings() {
    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        return;
    }

    auto http = network->CreateHttp(0);
    if (!http) {
        return;
    }

    std::string url = base_url_ + "/api/v1/agents/" + agent_id_ + "/voice";
    http->SetTimeout(5000);
    http->SetHeader("Authorization", "Bearer " + api_key_);

    if (auto opened = http->Open("GET", url); !opened) {
        return;
    }

    auto status = http->GetStatusCode();
    if (status && *status == 403) {
        ESP_LOGW(TAG,
                 "Agent %s voice check returned 403 VOICE_DISABLED. "
                 "Please ensure 'Voice Conversation' is toggled ON and published in the LangMind console.",
                 agent_id_.c_str());
    } else if (status && *status == 200) {
        ESP_LOGI(TAG, "Agent voice settings verified OK.");
    }
    http->Close();
}
