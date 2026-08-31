#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include "AudioEngine.hpp"
#include <memory>
#include <string>

namespace antigravity::core {

class TimelineController : public QObject {
    Q_OBJECT

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

private slots:
    void onClockTick();

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

    std::shared_ptr<AudioEngine> audio_engine_;

    QTimer clock_timer_;
    QElapsedTimer system_clock_;
};

} // namespace antigravity::core
