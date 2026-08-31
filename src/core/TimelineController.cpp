#include "TimelineController.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace antigravity::core {

QString TimelineController::secondsToSMPTE(double seconds, double fps) {
    if (std::isnan(seconds) || seconds < 0.0) seconds = 0.0;
    if (std::isnan(fps) || fps <= 0.0) fps = 30.0;

    int total_frames = static_cast<int>(std::floor(seconds * fps));
    int frames = total_frames % static_cast<int>(std::round(fps));
    int total_seconds = static_cast<int>(std::floor(seconds));
    int s = total_seconds % 60;
    int m = (total_seconds / 60) % 60;
    int h = total_seconds / 3600;

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", h, m, s, frames);
    return QString::fromLatin1(buf);
}

double TimelineController::smpteToSeconds(const QString& timecode, double fps) {
    auto parts = timecode.split(':');
    if (parts.size() != 4) return 0.0;
    int h = parts[0].toInt();
    int m = parts[1].toInt();
    int s = parts[2].toInt();
    int f = parts[3].toInt();
    return (h * 3600.0) + (m * 60.0) + s + (static_cast<double>(f) / fps);
}

TimelineController::TimelineController(QObject *parent)
    : QObject(parent) {
    connect(&clock_timer_, &QTimer::timeout, this, &TimelineController::onClockTick);
    clock_timer_.setTimerType(Qt::PreciseTimer);
}

TimelineController::~TimelineController() {
    clock_timer_.stop();
}

void TimelineController::setPosition(double pos) {
    seek(pos);
}

void TimelineController::setDuration(double dur) {
    if (qFuzzyCompare(duration_sec_, dur)) return;
    duration_sec_ = std::max(0.1, dur);
    out_point_ = duration_sec_;
    emit durationChanged();
    emit outPointChanged();
}

void TimelineController::setPlaybackRate(double rate) {
    if (qFuzzyCompare(playback_rate_, rate)) return;
    playback_rate_ = rate;
    emit playbackRateChanged();
}

void TimelineController::setFps(double fps) {
    if (qFuzzyCompare(fps_, fps)) return;
    fps_ = std::max(1.0, fps);
    emit fpsChanged();
    emit positionChanged();
}

QString TimelineController::timecode() const {
    return secondsToSMPTE(current_position_sec_, fps_);
}

void TimelineController::setVolume(float vol) {
    volume_ = std::clamp(vol, 0.0f, 2.0f);
    if (audio_engine_) {
        audio_engine_->setVolume(volume_);
    }
    emit volumeChanged();
}

void TimelineController::setMuted(bool mute) {
    is_muted_ = mute;
    if (audio_engine_) {
        audio_engine_->setMuted(is_muted_);
    }
    emit isMutedChanged();
}

void TimelineController::setInPoint(double pt) {
    in_point_ = std::clamp(pt, 0.0, out_point_);
    emit inPointChanged();
}

void TimelineController::setOutPoint(double pt) {
    out_point_ = std::clamp(pt, in_point_, duration_sec_);
    emit outPointChanged();
}

void TimelineController::setAudioEngine(std::shared_ptr<AudioEngine> audio_engine) {
    audio_engine_ = std::move(audio_engine);
    if (audio_engine_) {
        audio_engine_->setVolume(volume_);
        audio_engine_->setMuted(is_muted_);
    }
}

void TimelineController::play() {
    if (is_playing_) return;
    is_playing_ = true;
    system_clock_.restart();

    if (audio_engine_ && audio_engine_->metadata().has_audio && !is_muted_) {
        audio_engine_->play();
    }

    int interval_ms = static_cast<int>(1000.0 / std::max(30.0, fps_));
    clock_timer_.start(std::max(1, interval_ms));
    emit isPlayingChanged();
}

void TimelineController::pause() {
    if (!is_playing_) return;
    is_playing_ = false;
    clock_timer_.stop();

    if (audio_engine_) {
        audio_engine_->pause();
    }

    emit isPlayingChanged();
}

void TimelineController::togglePlay() {
    if (is_playing_) {
        pause();
    } else {
        play();
    }
}

void TimelineController::seek(double timestamp_sec) {
    current_position_sec_ = std::clamp(timestamp_sec, 0.0, duration_sec_);

    if (audio_engine_) {
        audio_engine_->seek(current_position_sec_);
    }

    system_clock_.restart();
    emit positionChanged();
    emit frameStepped(static_cast<int>(current_position_sec_ * fps_));
}

void TimelineController::stepFrame(int delta_frames) {
    pause();
    double step_sec = static_cast<double>(delta_frames) / fps_;
    seek(current_position_sec_ + step_sec);
}

void TimelineController::setShuttleRate(double rate) {
    setPlaybackRate(rate);
    if (!is_playing_ && rate != 0.0) {
        play();
    } else if (rate == 0.0) {
        pause();
    }
}

void TimelineController::jumpToStart() {
    seek(in_point_);
}

void TimelineController::jumpToEnd() {
    seek(out_point_);
}

void TimelineController::onClockTick() {
    updateClock();
}

void TimelineController::updateClock() {
    bool has_audio_clock = (audio_engine_ && audio_engine_->metadata().has_audio && !is_muted_ && playback_rate_ == 1.0);

    if (has_audio_clock) {
        // Master Clock driven by PipeWire audio hardware PTS
        double audio_pts = audio_engine_->getCurrentPTS();
        av_drift_ms_ = (current_position_sec_ - audio_pts) * 1000.0;
        current_position_sec_ = audio_pts;
        audio_latency_ms_ = audio_engine_->getAudioLatencyMs();
    } else {
        // Monotonic high-resolution system clock fallback
        double elapsed_sec = system_clock_.restart() / 1000.0;
        current_position_sec_ += (elapsed_sec * playback_rate_);
        av_drift_ms_ = 0.0;
        if (audio_engine_) {
            audio_latency_ms_ = audio_engine_->getAudioLatencyMs();
        }
    }

    // Range checking / Looping
    if (current_position_sec_ >= out_point_) {
        current_position_sec_ = in_point_;
        if (audio_engine_) {
            audio_engine_->seek(in_point_);
        }
    } else if (current_position_sec_ < in_point_) {
        current_position_sec_ = in_point_;
        if (audio_engine_) {
            audio_engine_->seek(in_point_);
        }
    }

    emit positionChanged();
    emit audioLatencyMsChanged();
    emit avDriftMsChanged();
    emit frameStepped(static_cast<int>(current_position_sec_ * fps_));
}

} // namespace antigravity::core
