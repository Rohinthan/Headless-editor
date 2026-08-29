#include "DecoderEngine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace antigravity::core {

namespace {
    AVPixelFormat g_hw_pix_fmt = AV_PIX_FMT_NONE;
}

AVPixelFormat DecoderEngine::get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
    (void)ctx;
    const AVPixelFormat* p = pix_fmts;
    while (*p != AV_PIX_FMT_NONE) {
        if (*p == g_hw_pix_fmt) {
            return *p;
        }
        p++;
    }
    // Fallback to the first available software format
    return pix_fmts[0];
}

std::vector<std::string> DecoderEngine::get_supported_hw_types() {
    std::vector<std::string> types;
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
        const char* name = av_hwdevice_get_type_name(type);
        if (name) {
            types.emplace_back(name);
        }
    }
    return types;
}

DecoderEngine::DecoderEngine() {
    packet_.reset(av_packet_alloc());
    frame_raw_.reset(av_frame_alloc());
    frame_sw_.reset(av_frame_alloc());
}

DecoderEngine::~DecoderEngine() {
    close();
}

DecoderEngine::DecoderEngine(DecoderEngine&& other) noexcept {
    *this = std::move(other);
}

DecoderEngine& DecoderEngine::operator=(DecoderEngine&& other) noexcept {
    if (this != &other) {
        close();
        std::unique_lock<std::mutex> lock(other.engine_mutex_);
        format_ctx_ = std::move(other.format_ctx_);
        codec_ctx_ = std::move(other.codec_ctx_);
        packet_ = std::move(other.packet_);
        frame_raw_ = std::move(other.frame_raw_);
        frame_sw_ = std::move(other.frame_sw_);
        hw_device_ctx_ = std::move(other.hw_device_ctx_);
        sws_ctx_ = std::move(other.sws_ctx_);
        video_stream_idx_ = other.video_stream_idx_;
        time_base_ = other.time_base_;
        hw_type_ = other.hw_type_;
        hw_pix_fmt_ = other.hw_pix_fmt_;
        metadata_ = std::move(other.metadata_);
        current_frame_index_ = other.current_frame_index_;
        last_sws_src_w_ = other.last_sws_src_w_;
        last_sws_src_h_ = other.last_sws_src_h_;
        last_sws_src_fmt_ = other.last_sws_src_fmt_;
        other.video_stream_idx_ = -1;
    }
    return *this;
}

bool DecoderEngine::init_hw_device(AVHWDeviceType type) {
    hw_device_ctx_.reset();
    AVBufferRef* hw_ctx = nullptr;
    int ret = av_hwdevice_ctx_create(&hw_ctx, type, nullptr, nullptr, 0);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        return false;
    }
    hw_device_ctx_.reset(hw_ctx);
    hw_type_ = type;
    return true;
}

bool DecoderEngine::setup_codec(bool prefer_hwaccel) {
    const AVStream* video_stream = format_ctx_->streams[video_stream_idx_];
    const AVCodec* decoder = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!decoder) {
        std::cerr << "[DecoderEngine] Decoder not found for codec id: "
                  << video_stream->codecpar->codec_id << std::endl;
        return false;
    }

    codec_ctx_.reset(avcodec_alloc_context3(decoder));
    if (!codec_ctx_) {
        return false;
    }

    if (avcodec_parameters_to_context(codec_ctx_.get(), video_stream->codecpar) < 0) {
        return false;
    }

    // Attempt hardware acceleration if preferred
    bool hw_init_success = false;
    if (prefer_hwaccel) {
        // Priority order: VAAPI -> CUDA -> DRM -> VDPAU -> VULKAN
        const std::vector<AVHWDeviceType> candidate_hw_types = {
            AV_HWDEVICE_TYPE_VAAPI,
            AV_HWDEVICE_TYPE_CUDA,
            AV_HWDEVICE_TYPE_DRM,
            AV_HWDEVICE_TYPE_VDPAU,
            AV_HWDEVICE_TYPE_VULKAN
        };

        for (auto candidate : candidate_hw_types) {
            // Find hw pixel format supported by this codec
            for (int i = 0;; i++) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
                if (!config) break;
                if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                    config->device_type == candidate) {
                    hw_pix_fmt_ = config->pix_fmt;
                    g_hw_pix_fmt = hw_pix_fmt_;
                    break;
                }
            }

            if (hw_pix_fmt_ != AV_PIX_FMT_NONE) {
                if (init_hw_device(candidate)) {
                    codec_ctx_->hw_device_ctx = av_buffer_ref(hw_device_ctx_.get());
                    codec_ctx_->get_format = DecoderEngine::get_hw_format;
                    hw_init_success = true;
                    break;
                }
            }
        }
    }

    // Multithreading options for software decode
    if (!hw_init_success) {
        codec_ctx_->thread_count = std::max(1u, std::thread::hardware_concurrency());
        codec_ctx_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        hw_pix_fmt_ = AV_PIX_FMT_NONE;
        g_hw_pix_fmt = AV_PIX_FMT_NONE;
        hw_type_ = AV_HWDEVICE_TYPE_NONE;
    }

    if (avcodec_open2(codec_ctx_.get(), decoder, nullptr) < 0) {
        std::cerr << "[DecoderEngine] Failed to open codec." << std::endl;
        return false;
    }

    // Fill metadata
    metadata_.filepath = format_ctx_->url ? format_ctx_->url : "";
    metadata_.format_name = format_ctx_->iformat->name ? format_ctx_->iformat->name : "unknown";
    metadata_.codec_name = decoder->name ? decoder->name : "unknown";
    metadata_.codec_long_name = decoder->long_name ? decoder->long_name : "";
    metadata_.width = codec_ctx_->width > 0 ? codec_ctx_->width : video_stream->codecpar->width;
    metadata_.height = codec_ctx_->height > 0 ? codec_ctx_->height : video_stream->codecpar->height;
    metadata_.raw_pix_fmt = codec_ctx_->pix_fmt;
    metadata_.bit_rate = format_ctx_->bit_rate > 0 ? format_ctx_->bit_rate : video_stream->codecpar->bit_rate;

    const char* profile = avcodec_profile_name(video_stream->codecpar->codec_id, video_stream->codecpar->profile);
    metadata_.profile_name = profile ? profile : "Main";

    if (video_stream->avg_frame_rate.den > 0 && video_stream->avg_frame_rate.num > 0) {
        metadata_.fps = av_q2d(video_stream->avg_frame_rate);
    } else if (video_stream->r_frame_rate.den > 0 && video_stream->r_frame_rate.num > 0) {
        metadata_.fps = av_q2d(video_stream->r_frame_rate);
    } else {
        metadata_.fps = 30.0;
    }

    if (format_ctx_->duration != AV_NOPTS_VALUE) {
        metadata_.duration_seconds = static_cast<double>(format_ctx_->duration) / AV_TIME_BASE;
    } else if (video_stream->duration != AV_NOPTS_VALUE) {
        metadata_.duration_seconds = static_cast<double>(video_stream->duration) * av_q2d(video_stream->time_base);
    } else {
        metadata_.duration_seconds = 0.0;
    }

    metadata_.total_frames = video_stream->nb_frames > 0
        ? video_stream->nb_frames
        : static_cast<int64_t>(metadata_.duration_seconds * metadata_.fps);

    metadata_.hw_accelerated = hw_init_success;
    if (hw_init_success) {
        metadata_.hw_device_name = av_hwdevice_get_type_name(hw_type_);
    } else {
        metadata_.hw_device_name = "None (CPU Software Fallback)";
    }

    return true;
}

bool DecoderEngine::open(const std::string& filepath, bool prefer_hwaccel) {
    close();
    std::unique_lock<std::mutex> lock(engine_mutex_);

    AVFormatContext* raw_fmt_ctx = nullptr;
    if (avformat_open_input(&raw_fmt_ctx, filepath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[DecoderEngine] Failed to open input file: " << filepath << std::endl;
        return false;
    }
    format_ctx_.reset(raw_fmt_ctx);

    if (avformat_find_stream_info(format_ctx_.get(), nullptr) < 0) {
        std::cerr << "[DecoderEngine] Failed to find stream info." << std::endl;
        close();
        return false;
    }

    video_stream_idx_ = av_find_best_stream(format_ctx_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx_ < 0) {
        std::cerr << "[DecoderEngine] No video stream found." << std::endl;
        close();
        return false;
    }

    time_base_ = format_ctx_->streams[video_stream_idx_]->time_base;

    if (!setup_codec(prefer_hwaccel)) {
        close();
        return false;
    }

    current_frame_index_ = 0;
    return true;
}

void DecoderEngine::close() {
    stop_streaming();
    std::unique_lock<std::mutex> lock(engine_mutex_);

    sws_ctx_.reset();
    hw_device_ctx_.reset();
    codec_ctx_.reset();
    format_ctx_.reset();

    video_stream_idx_ = -1;
    hw_type_ = AV_HWDEVICE_TYPE_NONE;
    hw_pix_fmt_ = AV_PIX_FMT_NONE;
    current_frame_index_ = 0;
    last_sws_src_w_ = 0;
    last_sws_src_h_ = 0;
    last_sws_src_fmt_ = AV_PIX_FMT_NONE;
    metadata_ = StreamMetadata{};
}

bool DecoderEngine::is_open() const {
    return format_ctx_ != nullptr && codec_ctx_ != nullptr && video_stream_idx_ >= 0;
}

bool DecoderEngine::process_frame(AVFrame* src_frame, DecodedVideoFrame& out_frame) {
    AVFrame* target_frame = src_frame;

    // If frame is in hardware surface, download it to CPU system memory
    if (src_frame->format == hw_pix_fmt_ && hw_pix_fmt_ != AV_PIX_FMT_NONE) {
        av_frame_unref(frame_sw_.get());
        int ret = av_hwframe_transfer_data(frame_sw_.get(), src_frame, 0);
        if (ret < 0) {
            std::cerr << "[DecoderEngine] Failed to transfer HW frame to CPU memory." << std::endl;
            return false;
        }
        target_frame = frame_sw_.get();
    }

    int w = target_frame->width;
    int h = target_frame->height;
    auto src_fmt = static_cast<AVPixelFormat>(target_frame->format);

    if (w <= 0 || h <= 0 || src_fmt == AV_PIX_FMT_NONE) {
        return false;
    }

    // Reinitialize SwsContext if resolution or format changed
    if (!sws_ctx_ || w != last_sws_src_w_ || h != last_sws_src_h_ || src_fmt != last_sws_src_fmt_) {
        sws_ctx_.reset(sws_getContext(
            w, h, src_fmt,
            w, h, AV_PIX_FMT_RGBA,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        ));
        if (!sws_ctx_) {
            std::cerr << "[DecoderEngine] Failed to create sws_scale context." << std::endl;
            return false;
        }
        last_sws_src_w_ = w;
        last_sws_src_h_ = h;
        last_sws_src_fmt_ = src_fmt;
    }

    out_frame.width = w;
    out_frame.height = h;
    out_frame.stride = w * 4;
    out_frame.rgba_data.resize(static_cast<size_t>(out_frame.stride * h));

    uint8_t* dst_data[4] = { out_frame.rgba_data.data(), nullptr, nullptr, nullptr };
    int dst_linesize[4] = { out_frame.stride, 0, 0, 0 };

    sws_scale(
        sws_ctx_.get(),
        target_frame->data,
        target_frame->linesize,
        0,
        h,
        dst_data,
        dst_linesize
    );

    int64_t pts = target_frame->pts != AV_NOPTS_VALUE ? target_frame->pts : target_frame->pkt_dts;
    if (pts == AV_NOPTS_VALUE) {
        pts = current_frame_index_;
    }

    out_frame.pts_raw = pts;
    out_frame.pts_seconds = static_cast<double>(pts) * av_q2d(time_base_);
    out_frame.frame_index = current_frame_index_++;
    out_frame.is_hw_accelerated = (src_frame->format == hw_pix_fmt_ && hw_pix_fmt_ != AV_PIX_FMT_NONE);
    out_frame.source_pixel_format = src_fmt;
    out_frame.hw_type = hw_type_;

    return true;
}

std::optional<DecodedVideoFrame> DecoderEngine::decode_next_frame() {
    std::unique_lock<std::mutex> lock(engine_mutex_);
    if (!is_open()) return std::nullopt;

    while (true) {
        // Try to receive a decoded frame from codec
        int ret = avcodec_receive_frame(codec_ctx_.get(), frame_raw_.get());
        if (ret == 0) {
            DecodedVideoFrame frame;
            if (process_frame(frame_raw_.get(), frame)) {
                av_frame_unref(frame_raw_.get());
                return frame;
            }
            av_frame_unref(frame_raw_.get());
            continue;
        }

        if (ret == AVERROR_EOF) {
            return std::nullopt;
        }

        if (ret != AVERROR(EAGAIN)) {
            char err[128];
            av_strerror(ret, err, sizeof(err));
            std::cerr << "[DecoderEngine] avcodec_receive_frame error: " << err << std::endl;
            return std::nullopt;
        }

        // Read next packet from container
        av_packet_unref(packet_.get());
        ret = av_read_frame(format_ctx_.get(), packet_.get());
        if (ret < 0) {
            // Flush decoder
            avcodec_send_packet(codec_ctx_.get(), nullptr);
            if (ret == AVERROR_EOF) {
                // Try one more receive
                ret = avcodec_receive_frame(codec_ctx_.get(), frame_raw_.get());
                if (ret == 0) {
                    DecodedVideoFrame frame;
                    if (process_frame(frame_raw_.get(), frame)) {
                        av_frame_unref(frame_raw_.get());
                        return frame;
                    }
                    av_frame_unref(frame_raw_.get());
                }
            }
            return std::nullopt;
        }

        if (packet_->stream_index == video_stream_idx_) {
            ret = avcodec_send_packet(codec_ctx_.get(), packet_.get());
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                char err[128];
                av_strerror(ret, err, sizeof(err));
                std::cerr << "[DecoderEngine] avcodec_send_packet error: " << err << std::endl;
            }
        }
        av_packet_unref(packet_.get());
    }
}

bool DecoderEngine::seek(double timestamp_seconds) {
    std::unique_lock<std::mutex> lock(engine_mutex_);
    if (!is_open()) return false;

    int64_t target_ts = static_cast<int64_t>(timestamp_seconds / av_q2d(time_base_));

    int ret = av_seek_frame(format_ctx_.get(), video_stream_idx_, target_ts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        // Retry with default flags
        ret = av_seek_frame(format_ctx_.get(), video_stream_idx_, target_ts, 0);
        if (ret < 0) {
            std::cerr << "[DecoderEngine] Seek failed to timestamp: " << timestamp_seconds << "s" << std::endl;
            return false;
        }
    }

    avcodec_flush_buffers(codec_ctx_.get());
    current_frame_index_ = static_cast<int64_t>(timestamp_seconds * metadata_.fps);

    if (ring_buffer_) {
        ring_buffer_->clear();
    }

    return true;
}

void DecoderEngine::start_streaming(size_t buffer_capacity) {
    if (is_streaming_) return;
    if (!is_open()) return;

    ring_buffer_ = std::make_unique<ThreadSafeRingBuffer<DecodedVideoFrame>>(buffer_capacity);
    is_streaming_ = true;
    streaming_thread_ = std::make_unique<std::thread>(&DecoderEngine::streaming_worker_loop, this);
}

void DecoderEngine::stop_streaming() {
    if (!is_streaming_) return;
    is_streaming_ = false;

    if (ring_buffer_) {
        ring_buffer_->stop();
    }

    if (streaming_thread_ && streaming_thread_->joinable()) {
        streaming_thread_->join();
    }
    streaming_thread_.reset();
    ring_buffer_.reset();
}

bool DecoderEngine::is_streaming() const {
    return is_streaming_.load();
}

std::optional<DecodedVideoFrame> DecoderEngine::get_next_buffered_frame() {
    if (!ring_buffer_) return std::nullopt;
    DecodedVideoFrame frame;
    if (ring_buffer_->pop(frame)) {
        return frame;
    }
    return std::nullopt;
}

void DecoderEngine::streaming_worker_loop() {
    while (is_streaming_) {
        if (seek_requested_.load()) {
            double target = seek_target_pts_.load();
            seek(target);
            seek_requested_ = false;
        }

        auto frame = decode_next_frame();
        if (!frame) {
            // End of stream or temporary stall
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (ring_buffer_) {
            ring_buffer_->push(std::move(*frame));
        }
    }
}

} // namespace antigravity::core
