#pragma once

#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QQuickWindow>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <mutex>
#include <atomic>
#include "../core/DecoderEngine.hpp"
#include "../core/GraphEngine.hpp"

namespace antigravity::ui {

class ViewportItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString hwAccelStatus READ hwAccelStatus NOTIFY hwAccelStatusChanged)
    Q_PROPERTY(int currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(int totalFrames READ totalFrames NOTIFY totalFramesChanged)

    // Interactive Transform & Effect Inspector properties
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double rotationAngle READ rotationAngle WRITE setRotationAngle NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double opacityValue READ opacityValue WRITE setOpacityValue NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double posX READ posX WRITE setPosX NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double posY READ posY WRITE setPosY NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double brightness READ brightness WRITE setBrightness NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double contrast READ contrast WRITE setContrast NOTIFY visualPropertiesChanged)
    Q_PROPERTY(double saturation READ saturation WRITE setSaturation NOTIFY visualPropertiesChanged)
    Q_PROPERTY(int blendModeIndex READ blendModeIndex WRITE setBlendModeIndex NOTIFY visualPropertiesChanged)

public:
    explicit ViewportItem(QQuickItem *parent = nullptr);
    ~ViewportItem() override;

    double position() const { return current_position_sec_; }
    void setPosition(double pos);

    double duration() const { return duration_sec_; }
    bool isPlaying() const { return is_playing_; }
    double fps() const { return fps_; }

    QString source() const { return source_path_; }
    void setSource(const QString &source);

    QString hwAccelStatus() const { return hw_accel_status_; }
    int currentFrame() const;
    int totalFrames() const;

    double scaleFactor() const { return scale_factor_; }
    void setScaleFactor(double v);

    double rotationAngle() const { return rotation_angle_; }
    void setRotationAngle(double v);

    double opacityValue() const { return opacity_val_; }
    void setOpacityValue(double v);

    double posX() const { return pos_x_; }
    void setPosX(double v);

    double posY() const { return pos_y_; }
    void setPosY(double v);

    double brightness() const { return brightness_; }
    void setBrightness(double v);

    double contrast() const { return contrast_; }
    void setContrast(double v);

    double saturation() const { return saturation_; }
    void setSaturation(double v);

    int blendModeIndex() const { return blend_mode_idx_; }
    void setBlendModeIndex(int idx);

    // Q_INVOKABLE Control Methods for QML
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void seek(double timestamp_sec);
    Q_INVOKABLE void stepForward();
    Q_INVOKABLE void stepBackward();
    Q_INVOKABLE void openFile(const QString& path);
    Q_INVOKABLE void resetTransformEffects();

signals:
    void positionChanged();
    void durationChanged();
    void isPlayingChanged();
    void fpsChanged();
    void sourceChanged();
    void hwAccelStatusChanged();
    void currentFrameChanged();
    void totalFramesChanged();
    void visualPropertiesChanged();
    void frameRendered(double pts, double renderTimeMs);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private slots:
    void onPlaybackTick();

private:
    void refreshFrame();
    void updateGraphParameters();

    std::unique_ptr<core::DecoderEngine> decoder_;
    std::unique_ptr<core::RenderGraph> graph_;

    QTimer playback_timer_;
    QElapsedTimer wall_clock_;

    double current_position_sec_ = 0.0;
    double duration_sec_ = 10.0;
    double fps_ = 30.0;
    bool is_playing_ = false;
    QString source_path_;
    QString hw_accel_status_ = "Initializing...";

    // Visual Parameters
    double scale_factor_ = 1.0;
    double rotation_angle_ = 0.0;
    double opacity_val_ = 1.0;
    double pos_x_ = 0.0;
    double pos_y_ = 0.0;
    double brightness_ = 0.0;
    double contrast_ = 1.0;
    double saturation_ = 1.0;
    int blend_mode_idx_ = 0;

    // Frame cache for QSG Texture creation
    QImage current_qimage_;
    std::mutex image_mutex_;
    std::atomic<bool> texture_dirty_{false};
};

} // namespace antigravity::ui
