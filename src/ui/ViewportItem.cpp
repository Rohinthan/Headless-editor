#include "ViewportItem.hpp"
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <iostream>

namespace antigravity::ui {

ViewportItem::ViewportItem(QQuickItem *parent)
    : QQuickItem(parent),
      decoder_(std::make_unique<core::DecoderEngine>()),
      graph_(std::make_unique<core::RenderGraph>()),
      gpu_engine_(std::make_unique<core::GPUEngine>()) {

    setFlag(ItemHasContents, true);

    // Default graph setup with test pattern generator
    graph_->setup_default_preset("");

    connect(&playback_timer_, &QTimer::timeout, this, &ViewportItem::onPlaybackTick);
    playback_timer_.setTimerType(Qt::PreciseTimer);

    // Initial probe of HW devices
    auto hw_types = core::DecoderEngine::get_supported_hw_types();
    if (!hw_types.empty()) {
        hw_accel_status_ = QString("HW Accelerated: %1 + OpenGL GLSL Pipeline").arg(QString::fromStdString(hw_types[0]));
    } else {
        hw_accel_status_ = "GPU GLSL Compositor Active";
    }

    refreshFrame();
}

ViewportItem::~ViewportItem() {
    playback_timer_.stop();
    decoder_->close();
}

void ViewportItem::setPosition(double pos) {
    seek(pos);
}

int ViewportItem::currentFrame() const {
    return static_cast<int>(current_position_sec_ * fps_);
}

int ViewportItem::totalFrames() const {
    return static_cast<int>(duration_sec_ * fps_);
}

void ViewportItem::setScaleFactor(double v) {
    if (qFuzzyCompare(scale_factor_, v)) return;
    scale_factor_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setRotationAngle(double v) {
    if (qFuzzyCompare(rotation_angle_, v)) return;
    rotation_angle_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setOpacityValue(double v) {
    if (qFuzzyCompare(opacity_val_, v)) return;
    opacity_val_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setPosX(double v) {
    if (qFuzzyCompare(pos_x_, v)) return;
    pos_x_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setPosY(double v) {
    if (qFuzzyCompare(pos_y_, v)) return;
    pos_y_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setBrightness(double v) {
    if (qFuzzyCompare(brightness_, v)) return;
    brightness_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setContrast(double v) {
    if (qFuzzyCompare(contrast_, v)) return;
    contrast_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setSaturation(double v) {
    if (qFuzzyCompare(saturation_, v)) return;
    saturation_ = v;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setBlendModeIndex(int idx) {
    if (blend_mode_idx_ == idx) return;
    blend_mode_idx_ = idx;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::updateGraphParameters() {
    auto t_node = std::dynamic_pointer_cast<core::TransformNode>(graph_->get_node("transform_1"));
    if (t_node) {
        t_node->scale_x.set_default_value(scale_factor_);
        t_node->scale_y.set_default_value(scale_factor_);
        t_node->rotation_deg.set_default_value(rotation_angle_);
        t_node->opacity.set_default_value(opacity_val_);
        t_node->pos_x.set_default_value(pos_x_);
        t_node->pos_y.set_default_value(pos_y_);
    }

    auto e_node = std::dynamic_pointer_cast<core::EffectNode>(graph_->get_node("effect_1"));
    if (e_node) {
        e_node->brightness.set_default_value(brightness_);
        e_node->contrast.set_default_value(contrast_);
        e_node->saturation.set_default_value(saturation_);
        e_node->blend_mode = static_cast<core::BlendMode>(blend_mode_idx_);
    }
}

void ViewportItem::resetTransformEffects() {
    scale_factor_ = 1.0;
    rotation_angle_ = 0.0;
    opacity_val_ = 1.0;
    pos_x_ = 0.0;
    pos_y_ = 0.0;
    brightness_ = 0.0;
    contrast_ = 1.0;
    saturation_ = 1.0;
    blend_mode_idx_ = 0;
    updateGraphParameters();
    emit visualPropertiesChanged();
    refreshFrame();
}

void ViewportItem::setSource(const QString &source) {
    if (source_path_ == source) return;
    openFile(source);
}

void ViewportItem::openFile(const QString& path) {
    QString local_file = path;
    if (path.startsWith("file://")) {
        local_file = QUrl(path).toLocalFile();
    }

    source_path_ = local_file;
    emit sourceChanged();

    bool opened = decoder_->open(local_file.toStdString(), true);
    if (opened) {
        duration_sec_ = std::max(0.1, decoder_->duration_seconds());
        fps_ = std::max(1.0, decoder_->fps());

        if (decoder_->is_hw_accelerated()) {
            hw_accel_status_ = QString("Hardware Accelerated (%1) + GPU GLSL Pipeline")
                .arg(QString::fromStdString(decoder_->hw_device_name()).toUpper());
        } else {
            hw_accel_status_ = "Multi-threaded CPU Decode + GPU GLSL Compositor";
        }

        graph_->setup_default_preset(local_file.toStdString());
        updateGraphParameters();

        emit durationChanged();
        emit fpsChanged();
        emit hwAccelStatusChanged();
        emit totalFramesChanged();

        current_position_sec_ = 0.0;
        emit positionChanged();
        emit currentFrameChanged();

        refreshFrame();
    } else {
        hw_accel_status_ = "Fallback to GPU Procedural Test Pattern";
        emit hwAccelStatusChanged();
        graph_->setup_default_preset("");
        refreshFrame();
    }
}

void ViewportItem::play() {
    if (is_playing_) return;
    is_playing_ = true;
    wall_clock_.restart();
    int interval_ms = static_cast<int>(1000.0 / std::max(1.0, fps_));
    playback_timer_.start(std::max(1, interval_ms));
    emit isPlayingChanged();
}

void ViewportItem::pause() {
    if (!is_playing_) return;
    is_playing_ = false;
    playback_timer_.stop();
    emit isPlayingChanged();
}

void ViewportItem::togglePlay() {
    if (is_playing_) {
        pause();
    } else {
        play();
    }
}

void ViewportItem::seek(double timestamp_sec) {
    current_position_sec_ = std::clamp(timestamp_sec, 0.0, duration_sec_);

    if (decoder_->is_open()) {
        decoder_->seek(current_position_sec_);
    }

    emit positionChanged();
    emit currentFrameChanged();
    refreshFrame();
}

void ViewportItem::stepForward() {
    pause();
    double delta = 1.0 / std::max(1.0, fps_);
    seek(current_position_sec_ + delta);
}

void ViewportItem::stepBackward() {
    pause();
    double delta = 1.0 / std::max(1.0, fps_);
    seek(current_position_sec_ - delta);
}

void ViewportItem::onPlaybackTick() {
    double elapsed_sec = wall_clock_.restart() / 1000.0;
    current_position_sec_ += elapsed_sec;

    if (current_position_sec_ >= duration_sec_) {
        current_position_sec_ = 0.0; // Loop playback
        if (decoder_->is_open()) {
            decoder_->seek(0.0);
        }
    }

    emit positionChanged();
    emit currentFrameChanged();
    refreshFrame();
}

void ViewportItem::refreshFrame() {
    QElapsedTimer render_timer;
    render_timer.start();

    int target_w = std::max(320, static_cast<int>(width()));
    int target_h = std::max(240, static_cast<int>(height()));

    auto frame_fetcher = [this](const std::string&, double) -> std::shared_ptr<core::DecodedVideoFrame> {
        if (!decoder_->is_open()) return nullptr;
        auto f = decoder_->decode_next_frame();
        if (f) {
            return std::make_shared<core::DecodedVideoFrame>(std::move(*f));
        }
        return nullptr;
    };

    core::RenderPlan plan = graph_->evaluate(current_position_sec_);
    plan.canvas_width = target_w;
    plan.canvas_height = target_h;

    // Use GPU Engine for compositing
    if (!gpu_initialized_) {
        gpu_initialized_ = gpu_engine_->initContext(target_w, target_h);
    }

    core::DecodedVideoFrame comp;
    if (gpu_initialized_) {
        gpu_engine_->makeCurrent();
        gpu_engine_->renderComposite(plan, frame_fetcher);
        comp = gpu_engine_->readbackOutputFrame(current_position_sec_);
    } else {
        comp = graph_->render_frame(current_position_sec_, target_w, target_h, frame_fetcher);
    }

    {
        std::lock_guard<std::mutex> lock(image_mutex_);
        current_qimage_ = QImage(
            comp.rgba_data.data(),
            comp.width,
            comp.height,
            comp.stride,
            QImage::Format_RGBA8888
        ).copy(); // Detach to own pixel buffer
        texture_dirty_ = true;
    }

    double render_time_ms = render_timer.nsecsElapsed() / 1000000.0;
    emit frameRendered(current_position_sec_, render_time_ms);
    update();
}

void ViewportItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        refreshFrame();
    }
}

QSGNode *ViewportItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
    }

    std::lock_guard<std::mutex> lock(image_mutex_);
    if (!current_qimage_.isNull() && window()) {
        if (texture_dirty_ || !node->texture()) {
            QSGTexture *texture = window()->createTextureFromImage(
                current_qimage_,
                QQuickWindow::TextureHasAlphaChannel
            );
            node->setTexture(texture);
            node->setOwnsTexture(true);
            texture_dirty_ = false;
        }

        // Calculate aspect-ratio preserving viewport rect
        double item_w = width();
        double item_h = height();
        double img_w = current_qimage_.width();
        double img_h = current_qimage_.height();

        if (img_w > 0 && img_h > 0 && item_w > 0 && item_h > 0) {
            double aspect = img_w / img_h;
            double target_w = item_w;
            double target_h = item_w / aspect;

            if (target_h > item_h) {
                target_h = item_h;
                target_w = item_h * aspect;
            }

            double offset_x = (item_w - target_w) * 0.5;
            double offset_y = (item_h - target_h) * 0.5;

            node->setRect(QRectF(offset_x, offset_y, target_w, target_h));
        } else {
            node->setRect(boundingRect());
        }
    }

    return node;
}

} // namespace antigravity::ui
