#ifndef LANGMIND_PROTOCOL_H
#define LANGMIND_PROTOCOL_H

#include "protocol.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class LangMindProtocol : public Protocol {
public:
    LangMindProtocol();
    ~LangMindProtocol() override;

    bool Start() override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel(bool send_goodbye = true) override;
    bool IsAudioChannelOpened() const override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    void SendStartListening(ListeningMode mode) override;
    void SendStopListening() override;
    void SendAbortSpeaking(AbortReason reason) override;
    void SendWakeWordDetected(const std::string& wake_word) override;
    void SendMcpMessage(const std::string& message) override;

    void ResetConversation();

private:
    std::string base_url_ = "https://langmind.in.th";
    std::string api_key_;
    std::string agent_id_ = "679fe16f-f04d-4dff-9c1a-3c9a9d7fd835";
    float voice_speed_ = 1.0f;
    std::string voice_override_;

    std::string conversation_id_;
    std::mutex mutex_;
    EventGroupHandle_t event_group_ = nullptr;
    TaskHandle_t worker_task_handle_ = nullptr;

    std::atomic<bool> is_recording_{false};
    std::atomic<bool> is_channel_opened_{false};
    std::atomic<bool> is_processing_{false};
    std::atomic<bool> abort_requested_{false};
    std::atomic<bool> worker_running_{false};
    std::atomic<bool> settings_checked_{false};

    ListeningMode listening_mode_ = kListeningModeAutoStop;
    std::vector<std::unique_ptr<AudioStreamPacket>> recorded_packets_;
    static constexpr size_t kMaxRecordedPackets = 250;  // 15 seconds @ 60ms/frame (~25 KB)
    static constexpr int64_t kSilenceThresholdUs = 1300000;  // 1.3 seconds of silence

    int64_t last_voice_detected_time_us_ = 0;
    int64_t recording_start_time_us_ = 0;
    bool voice_started_ = false;

    static void WorkerEntry(void* arg);
    void WorkerLoop();
    void TriggerProcessing();
    void HandleSpeechProcessing(std::vector<std::unique_ptr<AudioStreamPacket>> packets);

    std::string TranscribeAudio(const std::vector<std::unique_ptr<AudioStreamPacket>>& packets);
    std::string ChatMessage(const std::string& query);
    bool SynthesizeAndPlay(const std::string& text);
    void CheckVoiceSettings();

    static std::vector<std::string> SplitTextIntoChunks(const std::string& text, size_t max_bytes);

    bool SendText(const std::string& text) override;
};

#endif  // LANGMIND_PROTOCOL_H
