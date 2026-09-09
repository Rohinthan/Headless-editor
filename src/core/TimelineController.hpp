#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QProcess>
#include "AudioEngine.hpp"
#include <memory>
#include <string>
#include <vector>

namespace antigravity::core {

class TimelineController : public QObject {
    Q_OBJECT

    // Core Transport Properties
    Q_PROPERTY(double position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(double playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString timecode READ timecode NOTIFY positionChanged)
    Q_PROPERTY(double audioLatencyMs READ audioLatencyMs NOTIFY audioLatencyMsChanged)
    Q_PROPERTY(double avDriftMs READ avDriftMs NOTIFY avDriftMsChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool isMuted READ isMuted WRITE setMuted NOTIFY isMutedChanged)
    Q_PROPERTY(double inPoint READ inPoint WRITE setInPoint NOTIFY inPointChanged)
    Q_PROPERTY(double outPoint READ outPoint WRITE setOutPoint NOTIFY outPointChanged)

    // Project Properties
    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName NOTIFY projectChanged)
    Q_PROPERTY(QString aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY projectChanged)
    Q_PROPERTY(int canvasWidth READ canvasWidth WRITE setCanvasWidth NOTIFY projectChanged)
    Q_PROPERTY(int canvasHeight READ canvasHeight WRITE setCanvasHeight NOTIFY projectChanged)
    Q_PROPERTY(QString backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY projectChanged)

    // Export State Properties
    Q_PROPERTY(bool isExporting READ isExporting NOTIFY exportStateChanged)
    Q_PROPERTY(double exportProgress READ exportProgress NOTIFY exportProgressChanged)
    Q_PROPERTY(QString exportStatus READ exportStatus NOTIFY exportStateChanged)

public:
    explicit TimelineController(QObject *parent = nullptr);
    ~TimelineController() override;

    // Timecode Conversion Helpers
    static QString secondsToSMPTE(double seconds, double fps = 30.0);
    static double smpteToSeconds(const QString& timecode, double fps = 30.0);

    // Property Accessors
    double position() const { return current_position_sec_; }
    void setPosition(double pos);

    double duration() const { return duration_sec_; }
    void setDuration(double dur);

    bool isPlaying() const { return is_playing_; }
    double playbackRate() const { return playback_rate_; }
    void setPlaybackRate(double rate);

    double fps() const { return fps_; }
    void setFps(double fps);

    QString timecode() const;
    double audioLatencyMs() const { return audio_latency_ms_; }
    double avDriftMs() const { return av_drift_ms_; }

    float volume() const { return volume_; }
    void setVolume(float vol);

    bool isMuted() const { return is_muted_; }
    void setMuted(bool mute);

    double inPoint() const { return in_point_; }
    void setInPoint(double pt);

    double outPoint() const { return out_point_; }
    void setOutPoint(double pt);

    // Project Info Accessors
    QString projectName() const { return project_name_; }
    void setProjectName(const QString& name);

    QString aspectRatio() const { return aspect_ratio_; }
    void setAspectRatio(const QString& aspect);

    int canvasWidth() const { return canvas_width_; }
    void setCanvasWidth(int w);

    int canvasHeight() const { return canvas_height_; }
    void setCanvasHeight(int h);

    QString backgroundColor() const { return background_color_; }
    void setBackgroundColor(const QString& color);

    // Export Accessors
    bool isExporting() const { return is_exporting_; }
    double exportProgress() const { return export_progress_; }
    QString exportStatus() const { return export_status_; }

    // Audio Engine Binding
    void setAudioEngine(std::shared_ptr<AudioEngine> audio_engine);
    std::shared_ptr<AudioEngine> audioEngine() const { return audio_engine_; }

    // Q_INVOKABLE Control API
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void seek(double timestamp_sec);
    Q_INVOKABLE void stepFrame(int delta_frames);
    Q_INVOKABLE void setShuttleRate(double rate);
    Q_INVOKABLE void jumpToStart();
    Q_INVOKABLE void jumpToEnd();

    // Q_INVOKABLE Project Creation API
    Q_INVOKABLE void createProject(
        const QString& name,
        const QString& aspect,
        int width,
        int height,
        double fps,
        const QString& bgColor
    );

    // Q_INVOKABLE Clip Editing Operations (Cut, Split, Duplicate, Delete)
    Q_INVOKABLE void splitClipAtPlayhead(const QString& clipId);
    Q_INVOKABLE void duplicateClip(const QString& clipId);
    Q_INVOKABLE void deleteClip(const QString& clipId);

    // Q_INVOKABLE Export Rendering API
    Q_INVOKABLE void startExport(
        const QString& outputPath,
        const QString& format,
        int width,
        int height,
        double fps,
        int bitrateKbps
    );
    Q_INVOKABLE void cancelExport();

signals:
    void positionChanged();
    void durationChanged();
    void isPlayingChanged();
    void playbackRateChanged();
    void fpsChanged();
    void audioLatencyMsChanged();
    void avDriftMsChanged();
    void volumeChanged();
    void isMutedChanged();
    void inPointChanged();
    void outPointChanged();
    void frameStepped(int current_frame);

    // Project & Clip Signals
    void projectChanged();
    void clipSplitRequested(const QString& clipId, double splitTimestamp);
    void clipDuplicateRequested(const QString& clipId);
    void clipDeleteRequested(const QString& clipId);

    // Export Signals
    void exportStateChanged();
    void exportProgressChanged();
    void exportCompleted(bool success, const QString& message);

private slots:
    void onClockTick();
    void onExportProcessReadyRead();
    void onExportProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void updateClock();

    double current_position_sec_ = 0.0;
    double duration_sec_ = 30.0;
    double fps_ = 30.0;
    double playback_rate_ = 1.0;
    bool is_playing_ = false;

    float volume_ = 1.0f;
    bool is_muted_ = false;
    double in_point_ = 0.0;
    double out_point_ = 30.0;

    double audio_latency_ms_ = 0.0;
    double av_drift_ms_ = 0.0;

    // Project State
    QString project_name_ = "New Project";
    QString aspect_ratio_ = "16:9";
    int canvas_width_ = 1920;
    int canvas_height_ = 1080;
    QString background_color_ = "#000000";

    // Export State
    bool is_exporting_ = false;
    double export_progress_ = 0.0;
    QString export_status_ = "Idle";
    std::unique_ptr<QProcess> export_process_;
    double export_target_duration_ = 0.0;

    std::shared_ptr<AudioEngine> audio_engine_;

    QTimer clock_timer_;
    QElapsedTimer system_clock_;
};

} // namespace antigravity::core
