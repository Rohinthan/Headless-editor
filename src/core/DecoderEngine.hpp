#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <thread>
#include <optional>
#include <functional>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace antigravity::core {

// RAII Deleters for FFmpeg Structures
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const noexcept {
        if (ctx) {
            avformat_close_input(&ctx);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const noexcept {
        if (ctx) {
            avcodec_free_context(&ctx);
        }
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* pkt) const noexcept {
        if (pkt) {
            av_packet_free(&pkt);
        }
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const noexcept {
        if (frame) {
            av_frame_free(&frame);
        }
    }
};

struct AVBufferRefDeleter {
    void operator()(AVBufferRef* buf) const noexcept {
        if (buf) {
            av_buffer_unref(&buf);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* sws) const noexcept {
        if (sws) {
            sws_freeContext(sws);
        }
    }
};

// Aliases for RAII Managed FFmpeg Types
using ScopedAVFormatContext = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using ScopedAVCodecContext  = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using ScopedAVPacket        = std::unique_ptr<AVPacket, AVPacketDeleter>;
using ScopedAVFrame         = std::unique_ptr<AVFrame, AVFrameDeleter>;
using ScopedAVBufferRef     = std::unique_ptr<AVBufferRef, AVBufferRefDeleter>;
using ScopedSwsContext      = std::unique_ptr<SwsContext, SwsContextDeleter>;

// Struct representing a decoded and scaled RGBA video frame
struct DecodedVideoFrame {
    int width = 0;
    int height = 0;
    int stride = 0;
    double pts_seconds = 0.0;
    int64_t pts_raw = 0;
    int64_t frame_index = 0;
    std::vector<uint8_t> rgba_data;
    bool is_hw_accelerated = false;
    AVPixelFormat source_pixel_format = AV_PIX_FMT_NONE;
    AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
};

// Video Stream Metadata
struct StreamMetadata {
    std::string filepath;
    std::string format_name;
    std::string codec_name;
    std::string codec_long_name;
    std::string profile_name;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    double duration_seconds = 0.0;
    int64_t total_frames = 0;
    int64_t bit_rate = 0;
    AVPixelFormat raw_pix_fmt = AV_PIX_FMT_NONE;
    bool hw_accelerated = false;
    std::string hw_device_name = "None (CPU Software)";
};

// Thread-safe ring buffer for frame streaming
template<typename T>
class ThreadSafeRingBuffer {
public:
    explicit ThreadSafeRingBuffer(size_t capacity = 30)
        : capacity_(capacity) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this]() { return queue_.size() < capacity_ || stopped_; });
        if (stopped_) return;
        queue_.push_back(std::move(item));
        not_empty_.notify_one();
    }

    bool try_push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stopped_ || queue_.size() >= capacity_) {
            return false;
        }
        queue_.push_back(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !queue_.empty() || stopped_; });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop_front();
        not_full_.notify_one();
        return true;
    }

    bool try_pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop_front();
        not_full_.notify_one();
        return true;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.clear();
        not_full_.notify_all();
    }

    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    void reset() {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = false;
        queue_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T> queue_;
    size_t capacity_;
    std::atomic<bool> stopped_{false};
};

class DecoderEngine {
public:
    DecoderEngine();
    ~DecoderEngine();

    // Prevent copying
    DecoderEngine(const DecoderEngine&) = delete;
    DecoderEngine& operator=(const DecoderEngine&) = delete;

    // Moving is allowed
    DecoderEngine(DecoderEngine&&) noexcept;
    DecoderEngine& operator=(DecoderEngine&&) noexcept;

    // Core Demuxing & Decoding API
    bool open(const std::string& filepath, bool prefer_hwaccel = true);
    void close();
    bool is_open() const;

    // Direct synchronous frame decoding (ideal for benchmarks and random access)
    std::optional<DecodedVideoFrame> decode_next_frame();

    // Fast Seeking
    bool seek(double timestamp_seconds);

    // Background streaming thread management
    void start_streaming(size_t buffer_capacity = 30);
    void stop_streaming();
    bool is_streaming() const;
    std::optional<DecodedVideoFrame> get_next_buffered_frame();

    // Accessors
    const StreamMetadata& metadata() const { return metadata_; }
    double duration_seconds() const { return metadata_.duration_seconds; }
    double fps() const { return metadata_.fps; }
    int width() const { return metadata_.width; }
    int height() const { return metadata_.height; }
    bool is_hw_accelerated() const { return metadata_.hw_accelerated; }
    std::string hw_device_name() const { return metadata_.hw_device_name; }

    // Static probe helper
    static std::vector<std::string> get_supported_hw_types();

private:
    bool init_hw_device(AVHWDeviceType type);
    bool setup_codec(bool prefer_hwaccel);
    bool process_frame(AVFrame* src_frame, DecodedVideoFrame& out_frame);
    void streaming_worker_loop();

    static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);

    ScopedAVFormatContext format_ctx_;
    ScopedAVCodecContext  codec_ctx_;
    ScopedAVPacket        packet_;
    ScopedAVFrame         frame_raw_;
    ScopedAVFrame         frame_sw_;
    ScopedAVBufferRef     hw_device_ctx_;
    ScopedSwsContext      sws_ctx_;

    int video_stream_idx_ = -1;
    AVRational time_base_{1, 1000};
    AVHWDeviceType hw_type_ = AV_HWDEVICE_TYPE_NONE;
    AVPixelFormat hw_pix_fmt_ = AV_PIX_FMT_NONE;

    StreamMetadata metadata_;
    int64_t current_frame_index_ = 0;
    int last_sws_src_w_ = 0;
    int last_sws_src_h_ = 0;
    AVPixelFormat last_sws_src_fmt_ = AV_PIX_FMT_NONE;

    // Streaming thread members
    std::unique_ptr<ThreadSafeRingBuffer<DecodedVideoFrame>> ring_buffer_;
    std::unique_ptr<std::thread> streaming_thread_;
    std::atomic<bool> is_streaming_{false};
    std::atomic<bool> seek_requested_{false};
    std::atomic<double> seek_target_pts_{0.0};
    std::mutex engine_mutex_;
};

} // namespace antigravity::core
