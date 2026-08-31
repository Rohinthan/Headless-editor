#include "AudioEngine.hpp"
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace antigravity::core {

// ==================== AudioRingBuffer Implementation ====================

AudioRingBuffer::AudioRingBuffer(size_t capacity_samples)
    : buffer_(capacity_samples, 0.0f), capacity_(capacity_samples) {}

size_t AudioRingBuffer::write(const float* data, size_t count) {
    size_t write_idx = write_pos_.load(std::memory_order_relaxed);
    size_t read_idx = read_pos_.load(std::memory_order_acquire);

    size_t available = capacity_ - (write_idx - read_idx);
    size_t to_write = std::min(count, available);
    if (to_write == 0) return 0;

    size_t mask = capacity_ - 1;
    bool is_pow2 = (capacity_ & (capacity_ - 1)) == 0;

    for (size_t i = 0; i < to_write; ++i) {
        size_t idx = is_pow2 ? ((write_idx + i) & mask) : ((write_idx + i) % capacity_);
        buffer_[idx] = data[i];
    }

    write_pos_.store(write_idx + to_write, std::memory_order_release);
    return to_write;
}

size_t AudioRingBuffer::read(float* data, size_t count) {
    size_t read_idx = read_pos_.load(std::memory_order_relaxed);
    size_t write_idx = write_pos_.load(std::memory_order_acquire);

    size_t available = write_idx - read_idx;
    size_t to_read = std::min(count, available);
    if (to_read == 0) return 0;

    size_t mask = capacity_ - 1;
    bool is_pow2 = (capacity_ & (capacity_ - 1)) == 0;

    for (size_t i = 0; i < to_read; ++i) {
        size_t idx = is_pow2 ? ((read_idx + i) & mask) : ((read_idx + i) % capacity_);
        data[i] = buffer_[idx];
    }

    read_pos_.store(read_idx + to_read, std::memory_order_release);
    return to_read;
}

void AudioRingBuffer::clear() {
    read_pos_.store(0, std::memory_order_relaxed);
    write_pos_.store(0, std::memory_order_relaxed);
}

size_t AudioRingBuffer::available_read() const {
    size_t r = read_pos_.load(std::memory_order_relaxed);
    size_t w = write_pos_.load(std::memory_order_acquire);
    return (w >= r) ? (w - r) : 0;
}

size_t AudioRingBuffer::available_write() const {
    return capacity_ - available_read();
}

// ==================== AudioEngine Implementation ====================

namespace {

static const pw_stream_events g_stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = nullptr,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = AudioEngine::onProcessCallback,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr
};

} // namespace

void AudioEngine::onProcessCallback(void* userdata) {
    auto* self = static_cast<AudioEngine*>(userdata);
    if (!self || !self->pw_stream_) return;

    pw_buffer* b = pw_stream_dequeue_buffer(self->pw_stream_);
    if (!b) return;

    spa_buffer* buf = b->buffer;
    float* dst = static_cast<float*>(buf->datas[0].data);
    if (!dst) {
        pw_stream_queue_buffer(self->pw_stream_, b);
        return;
    }

    uint32_t maxsize = buf->datas[0].maxsize;
    uint32_t req_samples = maxsize / sizeof(float);

    if (self->is_playing_.load()) {
        size_t samples_read = self->ring_buffer_.read(dst, req_samples);

        // Underrun handling: fill remaining with silence
        if (samples_read < req_samples) {
            std::memset(dst + samples_read, 0, (req_samples - samples_read) * sizeof(float));
            self->underrun_count_.fetch_add(1, std::memory_order_relaxed);
        }

        // Apply Volume / Mute
        float vol = self->is_muted_.load() ? 0.0f : self->volume_.load();
        if (std::abs(vol - 1.0f) > 0.001f || vol == 0.0f) {
            for (uint32_t i = 0; i < samples_read; ++i) {
                dst[i] *= vol;
            }
        }

        self->samples_rendered_.fetch_add(samples_read, std::memory_order_relaxed);
    } else {
        std::memset(dst, 0, req_samples * sizeof(float));
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = 2 * sizeof(float); // Stereo float
    buf->datas[0].chunk->size = req_samples * sizeof(float);

    pw_stream_queue_buffer(self->pw_stream_, b);
}

AudioEngine::AudioEngine() {
    initPipeWire();
}

AudioEngine::~AudioEngine() {
    close();
    shutdownPipeWire();
}

bool AudioEngine::initPipeWire(const std::string& app_name) {
    if (is_connected_) return true;

    pw_init(nullptr, nullptr);

    pw_loop_ = pw_thread_loop_new("audio-render-loop", nullptr);
    if (!pw_loop_) {
        std::cerr << "[AudioEngine] Failed to create PipeWire thread loop." << std::endl;
        return false;
    }

    if (pw_thread_loop_start(pw_loop_) < 0) {
        std::cerr << "[AudioEngine] Failed to start PipeWire thread loop." << std::endl;
        pw_thread_loop_destroy(pw_loop_);
        pw_loop_ = nullptr;
        return false;
    }

    pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Production",
        PW_KEY_APP_NAME, app_name.c_str(),
        PW_KEY_NODE_LATENCY, "512/48000",
        nullptr
    );

    pw_thread_loop_lock(pw_loop_);

    pw_stream_ = pw_stream_new_simple(
        pw_thread_loop_get_loop(pw_loop_),
        app_name.c_str(),
        props,
        &g_stream_events,
        this
    );

    if (!pw_stream_) {
        pw_thread_loop_unlock(pw_loop_);
        std::cerr << "[AudioEngine] Failed to create PipeWire stream." << std::endl;
        shutdownPipeWire();
        return false;
    }

    // Audio format specification: 48kHz, Stereo, 32-bit Float
    uint8_t buffer[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    spa_audio_info_raw info = {};
    info.format = SPA_AUDIO_FORMAT_F32;
    info.channels = 2;
    info.rate = 48000;
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;

    const spa_pod* params[1];
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    int flags = PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS;
    if (pw_stream_connect(pw_stream_, PW_DIRECTION_OUTPUT, PW_ID_ANY, (pw_stream_flags)flags, params, 1) < 0) {
        pw_thread_loop_unlock(pw_loop_);
        std::cerr << "[AudioEngine] Failed to connect PipeWire stream." << std::endl;
        shutdownPipeWire();
        return false;
    }

    pw_thread_loop_unlock(pw_loop_);
    is_connected_ = true;
    return true;
}

void AudioEngine::shutdownPipeWire() {
    if (!is_connected_) return;
    is_connected_ = false;

    if (pw_loop_) {
        pw_thread_loop_lock(pw_loop_);
    }

    if (pw_stream_) {
        pw_stream_destroy(pw_stream_);
        pw_stream_ = nullptr;
    }

    if (pw_loop_) {
        pw_thread_loop_unlock(pw_loop_);
        pw_thread_loop_stop(pw_loop_);
        pw_thread_loop_destroy(pw_loop_);
        pw_loop_ = nullptr;
    }

    pw_deinit();
}

bool AudioEngine::open(const std::string& filepath) {
    close();

    AVFormatContext* raw_fmt = nullptr;
    if (avformat_open_input(&raw_fmt, filepath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    format_ctx_.reset(raw_fmt);

    if (avformat_find_stream_info(format_ctx_.get(), nullptr) < 0) {
        close();
        return false;
    }

    audio_stream_idx_ = av_find_best_stream(format_ctx_.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_idx_ < 0) {
        metadata_.has_audio = false;
        is_open_ = true;
        return true;
    }

    AVStream* stream = format_ctx_->streams[audio_stream_idx_];
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        close();
        return false;
    }

    codec_ctx_.reset(avcodec_alloc_context3(decoder));
    if (avcodec_parameters_to_context(codec_ctx_.get(), stream->codecpar) < 0) {
        close();
        return false;
    }

    if (avcodec_open2(codec_ctx_.get(), decoder, nullptr) < 0) {
        close();
        return false;
    }

    time_base_ = stream->time_base;

    // Configure libswresample to output Stereo 48kHz Float
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    AVChannelLayout in_ch_layout = codec_ctx_->ch_layout;

    SwrContext* raw_swr = nullptr;
    swr_alloc_set_opts2(
        &raw_swr,
        &out_ch_layout,
        AV_SAMPLE_FMT_FLT,
        48000,
        &in_ch_layout,
        codec_ctx_->sample_fmt,
        codec_ctx_->sample_rate,
        0, nullptr
    );

    if (!raw_swr || swr_init(raw_swr) < 0) {
        std::cerr << "[AudioEngine] Failed to initialize swresample context." << std::endl;
        close();
        return false;
    }
    swr_ctx_.reset(raw_swr);

    metadata_.codec_name = decoder->name ? decoder->name : "unknown";
    metadata_.source_sample_rate = codec_ctx_->sample_rate;
    metadata_.source_channels = codec_ctx_->ch_layout.nb_channels;
    metadata_.source_bit_rate = format_ctx_->bit_rate > 0 ? format_ctx_->bit_rate : stream->codecpar->bit_rate;
    metadata_.duration_seconds = static_cast<double>(format_ctx_->duration) / AV_TIME_BASE;
    metadata_.has_audio = true;

    is_open_ = true;
    feeder_running_ = true;
    feeder_thread_ = std::make_unique<std::thread>(&AudioEngine::audioFeederLoop, this);

    return true;
}

void AudioEngine::close() {
    is_playing_ = false;
    feeder_running_ = false;

    if (feeder_thread_ && feeder_thread_->joinable()) {
        feeder_thread_->join();
    }
    feeder_thread_.reset();

    swr_ctx_.reset();
    codec_ctx_.reset();
    format_ctx_.reset();

    audio_stream_idx_ = -1;
    is_open_ = false;
    samples_rendered_ = 0;
    ring_buffer_.clear();
    metadata_ = AudioMetadata{};
}

void AudioEngine::play() {
    is_playing_ = true;
}

void AudioEngine::pause() {
    is_playing_ = false;
}

void AudioEngine::seek(double timestamp_seconds) {
    seek_target_pts_.store(timestamp_seconds);
    samples_rendered_.store(static_cast<uint64_t>(timestamp_seconds * 48000.0 * 2.0));
    ring_buffer_.clear();
}

void AudioEngine::setVolume(float volume) {
    volume_.store(std::clamp(volume, 0.0f, 2.0f));
}

void AudioEngine::setMuted(bool muted) {
    is_muted_.store(muted);
}

double AudioEngine::getCurrentPTS() const {
    return static_cast<double>(samples_rendered_.load()) / (48000.0 * 2.0);
}

double AudioEngine::getAudioLatencyMs() const {
    // PipeWire native stream quantum latency: 512 frames / 48000 Hz = ~10.67 ms
    if (is_connected_) {
        return (512.0 / 48000.0) * 1000.0;
    }
    return 0.0;
}

void AudioEngine::generateTestTone(double duration_seconds, float frequency) {
    ring_buffer_.clear();
    samples_rendered_ = 0;

    size_t total_samples = static_cast<size_t>(duration_seconds * 48000.0);
    std::vector<float> pcm(total_samples * 2);

    for (size_t i = 0; i < total_samples; ++i) {
        double t = static_cast<double>(i) / 48000.0;
        float sample = 0.6f * std::sin(2.0 * M_PI * frequency * t)
                     + 0.2f * std::sin(2.0 * M_PI * frequency * 2.0 * t);

        pcm[i * 2 + 0] = sample;
        pcm[i * 2 + 1] = sample;
    }

    ring_buffer_.write(pcm.data(), pcm.size());
    metadata_.has_audio = true;
    metadata_.codec_name = "Synthetic Test Tone (48kHz Float)";
    metadata_.source_sample_rate = 48000;
    metadata_.source_channels = 2;
    metadata_.duration_seconds = duration_seconds;
}

void AudioEngine::audioFeederLoop() {
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    std::vector<float> resampled_buffer(4096 * 2);

    while (feeder_running_) {
        double seek_target = seek_target_pts_.exchange(-1.0);
        if (seek_target >= 0.0 && format_ctx_ && audio_stream_idx_ >= 0) {
            int64_t ts = static_cast<int64_t>(seek_target / av_q2d(time_base_));
            av_seek_frame(format_ctx_.get(), audio_stream_idx_, ts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codec_ctx_.get());
            ring_buffer_.clear();
        }

        if (ring_buffer_.available_write() < 48000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        int ret = av_read_frame(format_ctx_.get(), pkt);
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (pkt->stream_index == audio_stream_idx_) {
            if (avcodec_send_packet(codec_ctx_.get(), pkt) >= 0) {
                while (avcodec_receive_frame(codec_ctx_.get(), frame) >= 0) {
                    int out_samples = swr_get_out_samples(swr_ctx_.get(), frame->nb_samples);
                    if (static_cast<size_t>(out_samples * 2) > resampled_buffer.size()) {
                        resampled_buffer.resize(out_samples * 2);
                    }

                    uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(resampled_buffer.data()) };
                    int converted = swr_convert(
                        swr_ctx_.get(),
                        out_data,
                        out_samples,
                        (const uint8_t**)frame->data,
                        frame->nb_samples
                    );

                    if (converted > 0) {
                        ring_buffer_.write(resampled_buffer.data(), converted * 2);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
}

} // namespace antigravity::core
