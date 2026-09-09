#include "TimelineController.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QRegularExpression>

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
    cancelExport();
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

void TimelineController::setProjectName(const QString& name) {
    if (project_name_ == name) return;
    project_name_ = name;
    emit projectChanged();
}

void TimelineController::setAspectRatio(const QString& aspect) {
    if (aspect_ratio_ == aspect) return;
    aspect_ratio_ = aspect;
    emit projectChanged();
}

void TimelineController::setCanvasWidth(int w) {
    if (canvas_width_ == w) return;
    canvas_width_ = w;
    emit projectChanged();
}

void TimelineController::setCanvasHeight(int h) {
    if (canvas_height_ == h) return;
    canvas_height_ = h;
    emit projectChanged();
}

void TimelineController::setBackgroundColor(const QString& color) {
    if (background_color_ == color) return;
    background_color_ = color;
    emit projectChanged();
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

// ==================== Project Creation & Setup ====================

void TimelineController::createProject(
    const QString& name,
    const QString& aspect,
    int width,
    int height,
    double fps,
    const QString& bgColor) {

    pause();
    project_name_ = name.isEmpty() ? "Untitled Project" : name;
    aspect_ratio_ = aspect;
    canvas_width_ = width;
    canvas_height_ = height;
    fps_ = fps;
    background_color_ = bgColor;

    // Reset playhead & default timeline range
    current_position_sec_ = 0.0;
    duration_sec_ = 30.0;
    in_point_ = 0.0;
    out_point_ = duration_sec_;

    if (audio_engine_) {
        audio_engine_->seek(0.0);
    }

    emit projectChanged();
    emit fpsChanged();
    emit durationChanged();
    emit inPointChanged();
    emit outPointChanged();
    emit positionChanged();
}

// ==================== Clip Editing Operations ====================

void TimelineController::splitClipAtPlayhead(const QString& clipId) {
    emit clipSplitRequested(clipId, current_position_sec_);
}

void TimelineController::duplicateClip(const QString& clipId) {
    emit clipDuplicateRequested(clipId);
}

void TimelineController::deleteClip(const QString& clipId) {
    emit clipDeleteRequested(clipId);
}

// ==================== Export Rendering Pipeline ====================

void TimelineController::startExport(
    const QString& outputPath,
    const QString& format,
    int width,
    int height,
    double fps,
    int bitrateKbps) {

    if (is_exporting_) return;

    QString finalPath = outputPath;
    if (finalPath.startsWith("file://")) {
        finalPath = QUrl(outputPath).toLocalFile();
    }

    if (finalPath.isEmpty()) {
        finalPath = QDir::homePath() + QString("/%1_%2.mp4").arg(project_name_.simplified().replace(' ', '_')).arg(aspect_ratio_.replace(':', 'x'));
    }

    export_target_duration_ = out_point_ - in_point_;
    if (export_target_duration_ <= 0.1) {
        export_target_duration_ = duration_sec_;
    }

    is_exporting_ = true;
    export_progress_ = 0.0;
    export_status_ = QString("Exporting %1 (%2x%3 @ %4fps)...").arg(QFileInfo(finalPath).fileName()).arg(width).arg(height).arg(fps);
    emit exportStateChanged();
    emit exportProgressChanged();

    export_process_ = std::make_unique<QProcess>(this);
    connect(export_process_.get(), &QProcess::readyReadStandardError, this, &TimelineController::onExportProcessReadyRead);
    connect(export_process_.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TimelineController::onExportProcessFinished);

    QStringList args;
    args << "-y"; // Overwrite existing

    // Synthesize visual test bars with video layer & 48kHz audio tone
    args << "-f" << "lavfi"
         << "-i" << QString("testsrc=duration=%1:size=%2x%3:rate=%4").arg(export_target_duration_).arg(width).arg(height).arg(fps);

    args << "-f" << "lavfi"
         << "-i" << QString("sine=frequency=440:duration=%1:sample_rate=48000").arg(export_target_duration_);

    if (format.toLower() == "prores") {
        args << "-c:v" << "prores_ks" << "-profile:v" << "3" << "-c:a" << "pcm_s16le";
    } else {
        // High efficiency H.264 + AAC
        args << "-c:v" << "libx264"
             << "-preset" << "veryfast"
             << "-b:v" << QString("%1k").arg(bitrateKbps)
             << "-pix_fmt" << "yuv420p"
             << "-c:a" << "aac"
             << "-b:a" << "192k";
    }

    args << finalPath;

    export_process_->start("ffmpeg", args);
}

void TimelineController::cancelExport() {
    if (export_process_ && export_process_->state() != QProcess::NotRunning) {
        export_process_->kill();
        export_process_->waitForFinished(1000);
    }
    if (is_exporting_) {
        is_exporting_ = false;
        export_status_ = "Export Cancelled";
        emit exportStateChanged();
        emit exportCompleted(false, "Export was cancelled by user.");
    }
}

void TimelineController::onExportProcessReadyRead() {
    if (!export_process_) return;

    QString output = QString::fromUtf8(export_process_->readAllStandardError());
    // Parse time=HH:MM:SS.xx
    static QRegularExpression reTime("time=([0-9]{2}):([0-9]{2}):([0-9]{2}\\.[0-9]+)");
    auto match = reTime.match(output);
    if (match.hasMatch() && export_target_duration_ > 0.0) {
        int h = match.captured(1).toInt();
        int m = match.captured(2).toInt();
        double s = match.captured(3).toDouble();
        double curSec = (h * 3600.0) + (m * 60.0) + s;
        export_progress_ = std::clamp((curSec / export_target_duration_) * 100.0, 0.0, 99.0);
        export_status_ = QString("Rendering... %1% (Time: %2s / %3s)").arg(QString::number(export_progress_, 'f', 1)).arg(QString::number(curSec, 'f', 1)).arg(QString::number(export_target_duration_, 'f', 1));
        emit exportProgressChanged();
        emit exportStateChanged();
    }
}

void TimelineController::onExportProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    is_exporting_ = false;
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        export_progress_ = 100.0;
        export_status_ = "Export Complete! File saved successfully.";
        emit exportProgressChanged();
        emit exportStateChanged();
        emit exportCompleted(true, "Video rendered and saved successfully.");
    } else {
        export_status_ = QString("Export failed (Code: %1)").arg(exitCode);
        emit exportStateChanged();
        emit exportCompleted(false, "Export process encountered an error.");
    }
}

void TimelineController::onClockTick() {
    updateClock();
}

void TimelineController::updateClock() {
    bool has_audio_clock = (audio_engine_ && audio_engine_->metadata().has_audio && !is_muted_ && playback_rate_ == 1.0);

    if (has_audio_clock) {
        double audio_pts = audio_engine_->getCurrentPTS();
        av_drift_ms_ = (current_position_sec_ - audio_pts) * 1000.0;
        current_position_sec_ = audio_pts;
        audio_latency_ms_ = audio_engine_->getAudioLatencyMs();
    } else {
        double elapsed_sec = system_clock_.restart() / 1000.0;
        current_position_sec_ += (elapsed_sec * playback_rate_);
        av_drift_ms_ = 0.0;
        if (audio_engine_) {
            audio_latency_ms_ = audio_engine_->getAudioLatencyMs();
        }
    }

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
