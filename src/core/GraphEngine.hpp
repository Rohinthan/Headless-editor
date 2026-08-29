#pragma once

#include "DecoderEngine.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <cmath>
#include <algorithm>
#include <functional>

namespace antigravity::core {

// 2D Vector
struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
};

// Bézier Control Points for cubic easing curves: P0=(0,0), P1=(x1,y1), P2=(x2,y2), P3=(1,1)
struct BezierControlPoints {
    double x1 = 0.25;
    double y1 = 0.10;
    double x2 = 0.25;
    double y2 = 1.00;

    static BezierControlPoints Linear() { return {0.0, 0.0, 1.0, 1.0}; }
    static BezierControlPoints Ease() { return {0.25, 0.1, 0.25, 1.0}; }
    static BezierControlPoints EaseIn() { return {0.42, 0.0, 1.0, 1.0}; }
    static BezierControlPoints EaseOut() { return {0.0, 0.0, 0.58, 1.0}; }
    static BezierControlPoints EaseInOut() { return {0.42, 0.0, 0.58, 1.0}; }
};

enum class EasingType {
    Linear,
    Ease,
    EaseIn,
    EaseOut,
    EaseInOut,
    Custom
};

// Pure mathematical cubic Bézier curve evaluator:
// P(t) = (1-t)^3 * P0 + 3*(1-t)^2 * t * P1 + 3*(1-t) * t^2 * P2 + t^3 * P3
inline double evaluateCubicBezierRaw(double t, double p0, double p1, double p2, double p3) {
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    return uuu * p0 + 3.0 * uu * t * p1 + 3.0 * u * tt * p2 + ttt * p3;
}

inline Vec2 evaluateCubicBezier2D(double t, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3) {
    return {
        evaluateCubicBezierRaw(t, p0.x, p1.x, p2.x, p3.x),
        evaluateCubicBezierRaw(t, p0.y, p1.y, p2.y, p3.y)
    };
}

// Solves cubic Bézier timing curve for normalized time s in [0, 1]
double solveCubicBezierTiming(double s, const BezierControlPoints& cp);

// Keyframe structure for animating any scalar property
struct Keyframe {
    double time_seconds = 0.0;
    double value = 0.0;
    EasingType easing_type = EasingType::EaseInOut;
    BezierControlPoints custom_cp = BezierControlPoints::EaseInOut();

    bool operator<(const Keyframe& other) const {
        return time_seconds < other.time_seconds;
    }
};

// Keyframe Track with cubic Bézier interpolation
class KeyframeTrack {
public:
    explicit KeyframeTrack(double default_value = 0.0);

    void add_keyframe(double time_sec, double value, EasingType easing = EasingType::EaseInOut, BezierControlPoints custom_cp = {});
    void remove_keyframe(double time_sec, double tolerance = 0.001);
    void clear();

    double evaluate(double time_sec) const;
    const std::vector<Keyframe>& keyframes() const { return keyframes_; }
    double default_value() const { return default_value_; }
    void set_default_value(double val) { default_value_ = val; }
    bool has_keyframes() const { return !keyframes_.empty(); }

private:
    double default_value_ = 0.0;
    std::vector<Keyframe> keyframes_;
};

enum class BlendMode {
    Normal,
    Add,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten
};

struct TransformState {
    double pos_x = 0.0;
    double pos_y = 0.0;
    double scale_x = 1.0;
    double scale_y = 1.0;
    double rotation_deg = 0.0;
    double opacity = 1.0;
    double anchor_x = 0.5;
    double anchor_y = 0.5;
};

struct EffectState {
    double brightness = 0.0;  // -1.0 to +1.0
    double contrast = 1.0;    // 0.0 to 3.0
    double saturation = 1.0;  // 0.0 to 3.0
    double gamma = 1.0;       // 0.1 to 3.0
    double tint_r = 1.0;
    double tint_g = 1.0;
    double tint_b = 1.0;
    double blur_radius = 0.0;
    BlendMode blend_mode = BlendMode::Normal;
};

struct ClipState {
    std::string clip_id;
    std::string media_path;
    double start_time = 0.0;
    double duration = 10.0;
    double in_point = 0.0;
    double speed = 1.0;
    int layer_index = 0;
    bool is_generator = false;
    uint32_t generator_color = 0xFF336699;
};

struct LayerRenderInstruction {
    ClipState clip;
    TransformState transform;
    EffectState effect;
    double source_time_offset = 0.0;
    bool is_visible = true;
};

struct RenderPlan {
    double timestamp_seconds = 0.0;
    int canvas_width = 1920;
    int canvas_height = 1080;
    uint32_t background_color = 0xFF121214; // ARGB dark background
    std::vector<LayerRenderInstruction> layers;
};

// DAG Base Node
class Node {
public:
    enum class Type {
        Clip,
        Transform,
        Effect,
        Composite,
        Output
    };

    Node(std::string id, std::string name, Type type);
    virtual ~Node() = default;

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    Type type() const { return type_; }

    virtual void evaluate_node(double timestamp_sec, RenderPlan& plan) = 0;

protected:
    std::string id_;
    std::string name_;
    Type type_;
};

// Concrete Media Clip Node
class ClipNode : public Node {
public:
    ClipNode(std::string id, std::string name, ClipState state);
    void evaluate_node(double timestamp_sec, RenderPlan& plan) override;

    ClipState& state() { return state_; }
    const ClipState& state() const { return state_; }

private:
    ClipState state_;
};

// Concrete Transform Node
class TransformNode : public Node {
public:
    TransformNode(std::string id, std::string name);
    void evaluate_node(double timestamp_sec, RenderPlan& plan) override;

    KeyframeTrack pos_x;
    KeyframeTrack pos_y;
    KeyframeTrack scale_x;
    KeyframeTrack scale_y;
    KeyframeTrack rotation_deg;
    KeyframeTrack opacity;
    KeyframeTrack anchor_x;
    KeyframeTrack anchor_y;

    TransformState evaluate_transform(double timestamp_sec) const;
};

// Concrete Effect Node
class EffectNode : public Node {
public:
    EffectNode(std::string id, std::string name);
    void evaluate_node(double timestamp_sec, RenderPlan& plan) override;

    KeyframeTrack brightness;
    KeyframeTrack contrast;
    KeyframeTrack saturation;
    KeyframeTrack gamma;
    KeyframeTrack tint_r;
    KeyframeTrack tint_g;
    KeyframeTrack tint_b;
    KeyframeTrack blur_radius;
    BlendMode blend_mode = BlendMode::Normal;

    EffectState evaluate_effect(double timestamp_sec) const;
};

// Concrete Composite / Output Node
class CompositeNode : public Node {
public:
    CompositeNode(std::string id, std::string name, int width = 1920, int height = 1080);
    void evaluate_node(double timestamp_sec, RenderPlan& plan) override;

    int width() const { return width_; }
    int height() const { return height_; }
    void set_resolution(int w, int h) { width_ = w; height_ = h; }

private:
    int width_;
    int height_;
};

// Render Graph DAG Engine
class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    // Node Management
    void add_node(std::shared_ptr<Node> node);
    void remove_node(const std::string& node_id);
    std::shared_ptr<Node> get_node(const std::string& node_id) const;

    // Directed Connection Management
    bool connect_nodes(const std::string& from_id, const std::string& to_id);
    void disconnect_nodes(const std::string& from_id, const std::string& to_id);
    void clear();

    // DAG Cycle Detection and Topological Sort
    bool has_cycle() const;
    std::vector<std::shared_ptr<Node>> get_topological_order() const;

    // Evaluate DAG at timestamp -> returns lightweight render plan
    RenderPlan evaluate(double timestamp_seconds);

    // High performance CPU / Compositing rasterizer
    DecodedVideoFrame render_frame(
        double timestamp_seconds,
        int output_width,
        int output_height,
        const std::function<std::shared_ptr<DecodedVideoFrame>(const std::string&, double)>& frame_fetcher = nullptr
    );

    // Create a default timeline preset
    void setup_default_preset(const std::string& media_path = "");

private:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> adjacency_list_;
    std::unordered_map<std::string, std::vector<std::string>> in_edges_;
    int canvas_width_ = 1920;
    int canvas_height_ = 1080;
};

} // namespace antigravity::core
