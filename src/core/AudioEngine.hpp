#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>

namespace antigravity::core {

// Lock-Free Single-Producer Single-Consumer (SPSC) Audio Ring Buffer
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity_samples = 48000 * 2 * 4); // ~4 seconds of stereo 48kHz
    ~AudioRingBuffer() = default;

    size_t write(const float* data, size_t count);
    size_t read(float* data, size_t count);
    void clear();

    size_t available_read() const;
    size_t available_write() const;
    size_t capacity() const { return capacity_; }

private:
    std::vector<float> buffer_;
    size_t capacity_;
    std::atomic<size_t> write_pos_{0};
    std::atomic<size_t> read_pos_{0};
};

struct AudioMetadata {
    std::string codec_name;
    int source_sample_rate = 0;
    int source_channels = 0;
    int64_t source_bit_rate = 0;
    double duration_seconds = 0.0;
    bool has_audio = false;
};

// RAII SwrContext Deleter
struct SwrContextDeleter {
    void operator()(SwrContext* ctx) const noexcept {
        if (ctx) {
            swr_free(&ctx);
        }
    }
};
using ScopedSwrContext = std::unique_ptr<SwrContext, SwrContextDeleter>;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    // PipeWire Daemon Connection
    bool initPipeWire(const std::string& app_name = "Headless-Editor Audio Engine");
    void shutdownPipeWire();
    bool isConnected() const { return is_connected_.load(); }

    // Media Demuxing & Decoding Setup
    bool open(const std::string& filepath);
    void close();
    bool isOpen() const { return is_open_; }

    // Playback Controls
    void play();
    void pause();
    bool isPlaying() const { return is_playing_.load(); }
    void seek(double timestamp_seconds);

    // Volume & Gain [0.0 - 2.0]
    void setVolume(float volume);
    float volume() const { return volume_.load(); }
    void setMuted(bool muted);
    bool isMuted() const { return is_muted_.load(); }

    // Master Clock & Telemetry
    double getCurrentPTS() const;
    double getAudioLatencyMs() const;
    uint64_t getUnderrunCount() const { return underrun_count_.load(); }
    const AudioMetadata& metadata() const { return metadata_; }

    // Procedural Audio Generator (for testing or audio-less clips)
    void generateTestTone(double duration_seconds = 5.0, float frequency = 440.0f);

    // Static PipeWire Callback Hook
    static void onProcessCallback(void* userdata);

private:
    void audioFeederLoop();

    // PipeWire Native Handles
    pw_thread_loop* pw_loop_ = nullptr;
    pw_stream* pw_stream_ = nullptr;
    std::atomic<bool> is_connected_{false};

    // FFmpeg Audio Decoders
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> format_ctx_{nullptr, [](AVFormatContext* p){ if(p) avformat_close_input(&p); }};
    std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)> codec_ctx_{nullptr, [](AVCodecContext* p){ if(p) avcodec_free_context(&p); }};
    ScopedSwrContext swr_ctx_;
    int audio_stream_idx_ = -1;
    AVRational time_base_{1, 48000};
    AudioMetadata metadata_;

    // Lock-free buffer & feeder thread
    AudioRingBuffer ring_buffer_;
    std::unique_ptr<std::thread> feeder_thread_;
    std::atomic<bool> feeder_running_{false};
    std::atomic<bool> is_open_{false};
    std::atomic<bool> is_playing_{false};
    std::atomic<bool> is_muted_{false};
    std::atomic<float> volume_{1.0f};

    // Clock synchronization state
    std::atomic<uint64_t> samples_rendered_{0};
    std::atomic<uint64_t> underrun_count_{0};
    std::atomic<double> seek_target_pts_{-1.0};
    std::mutex feeder_mutex_;
};

} // namespace antigravity::core
