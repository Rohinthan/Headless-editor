#include "GraphEngine.hpp"
#include <queue>
#include <stack>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace antigravity::core {

namespace {

// Derivative of cubic bezier x component with respect to parameter u
inline double cubicBezierXDerivative(double u, double x1, double x2) {
    // X(u) = 3*(1-u)^2*u*x1 + 3*(1-u)*u^2*x2 + u^3
    // dX/du = 3*(1-u)^2*x1 + 6*(1-u)*u*(x2 - x1) + 3*u^2*(1 - x2)
    double one_minus_u = 1.0 - u;
    return 3.0 * one_minus_u * one_minus_u * x1 +
           6.0 * one_minus_u * u * (x2 - x1) +
           3.0 * u * u * (1.0 - x2);
}

// X component of cubic bezier at parameter u
inline double cubicBezierX(double u, double x1, double x2) {
    double one_minus_u = 1.0 - u;
    return 3.0 * one_minus_u * one_minus_u * u * x1 +
           3.0 * one_minus_u * u * u * x2 +
           u * u * u;
}

// Y component of cubic bezier at parameter u
inline double cubicBezierY(double u, double y1, double y2) {
    double one_minus_u = 1.0 - u;
    return 3.0 * one_minus_u * one_minus_u * u * y1 +
           3.0 * one_minus_u * u * u * y2 +
           u * u * u;
}

// Clamp helper
template<typename T>
inline T clampVal(T v, T min_v, T max_v) {
    return std::max(min_v, std::min(v, max_v));
}

} // namespace

double solveCubicBezierTiming(double s, const BezierControlPoints& cp) {
    s = clampVal(s, 0.0, 1.0);
    if (s <= 0.0) return 0.0;
    if (s >= 1.0) return 1.0;

    // Fast path for linear
    if (std::abs(cp.x1 - cp.y1) < 1e-5 && std::abs(cp.x2 - cp.y2) < 1e-5 &&
        std::abs(cp.x1 - 0.0) < 1e-5 && std::abs(cp.x2 - 1.0) < 1e-5) {
        return s;
    }

    // Newton-Raphson iteration to solve X(u) = s
    double u = s; // Initial guess
    for (int i = 0; i < 8; ++i) {
        double current_x = cubicBezierX(u, cp.x1, cp.x2) - s;
        if (std::abs(current_x) < 1e-6) {
            return cubicBezierY(u, cp.y1, cp.y2);
        }
        double dxdt = cubicBezierXDerivative(u, cp.x1, cp.x2);
        if (std::abs(dxdt) < 1e-6) {
            break; // Derivative too small, switch to binary search
        }
        u -= current_x / dxdt;
        u = clampVal(u, 0.0, 1.0);
    }

    // Binary search fallback
    double low = 0.0;
    double high = 1.0;
    u = s;
    while (low < high) {
        double current_x = cubicBezierX(u, cp.x1, cp.x2);
        if (std::abs(current_x - s) < 1e-6) {
            return cubicBezierY(u, cp.y1, cp.y2);
        }
        if (s > current_x) {
            low = u;
        } else {
            high = u;
        }
        u = (high + low) * 0.5;
        if (std::abs(high - low) < 1e-6) break;
    }

    return cubicBezierY(u, cp.y1, cp.y2);
}

// ---------------- KeyframeTrack Implementation ----------------

KeyframeTrack::KeyframeTrack(double default_value)
    : default_value_(default_value) {}

void KeyframeTrack::add_keyframe(double time_sec, double value, EasingType easing, BezierControlPoints custom_cp) {
    remove_keyframe(time_sec); // Replace if already exists at this timestamp

    BezierControlPoints cp;
    switch (easing) {
        case EasingType::Linear:    cp = BezierControlPoints::Linear(); break;
        case EasingType::Ease:      cp = BezierControlPoints::Ease(); break;
        case EasingType::EaseIn:    cp = BezierControlPoints::EaseIn(); break;
        case EasingType::EaseOut:   cp = BezierControlPoints::EaseOut(); break;
        case EasingType::EaseInOut: cp = BezierControlPoints::EaseInOut(); break;
        case EasingType::Custom:    cp = custom_cp; break;
    }

    keyframes_.push_back({time_sec, value, easing, cp});
    std::sort(keyframes_.begin(), keyframes_.end());
}

void KeyframeTrack::remove_keyframe(double time_sec, double tolerance) {
    keyframes_.erase(
        std::remove_if(keyframes_.begin(), keyframes_.end(), [time_sec, tolerance](const Keyframe& k) {
            return std::abs(k.time_seconds - time_sec) <= tolerance;
        }),
        keyframes_.end()
    );
}

void KeyframeTrack::clear() {
    keyframes_.clear();
}

double KeyframeTrack::evaluate(double time_sec) const {
    if (keyframes_.empty()) {
        return default_value_;
    }

    if (keyframes_.size() == 1) {
        return keyframes_.front().value;
    }

    if (time_sec <= keyframes_.front().time_seconds) {
        return keyframes_.front().value;
    }

    if (time_sec >= keyframes_.back().time_seconds) {
        return keyframes_.back().value;
    }

    // Find the bounding keyframe segment
    for (size_t i = 0; i < keyframes_.size() - 1; ++i) {
        const auto& k0 = keyframes_[i];
        const auto& k1 = keyframes_[i + 1];

        if (time_sec >= k0.time_seconds && time_sec <= k1.time_seconds) {
            double span = k1.time_seconds - k0.time_seconds;
            if (span <= 1e-7) return k0.value;

            double normalized_t = (time_sec - k0.time_seconds) / span;
            double eased_t = solveCubicBezierTiming(normalized_t, k0.custom_cp);

            return k0.value + eased_t * (k1.value - k0.value);
        }
    }

    return keyframes_.back().value;
}

// ---------------- Node Implementations ----------------

Node::Node(std::string id, std::string name, Type type)
    : id_(std::move(id)), name_(std::move(name)), type_(type) {}

ClipNode::ClipNode(std::string id, std::string name, ClipState state)
    : Node(std::move(id), std::move(name), Type::Clip), state_(std::move(state)) {}

void ClipNode::evaluate_node(double timestamp_sec, RenderPlan& plan) {
    if (timestamp_sec >= state_.start_time && timestamp_sec < (state_.start_time + state_.duration)) {
        LayerRenderInstruction layer;
        layer.clip = state_;
        layer.source_time_offset = (timestamp_sec - state_.start_time) * state_.speed + state_.in_point;
        layer.is_visible = true;
        plan.layers.push_back(layer);
    }
}

TransformNode::TransformNode(std::string id, std::string name)
    : Node(std::move(id), std::move(name), Type::Transform),
      pos_x(0.0), pos_y(0.0), scale_x(1.0), scale_y(1.0),
      rotation_deg(0.0), opacity(1.0), anchor_x(0.5), anchor_y(0.5) {}

TransformState TransformNode::evaluate_transform(double timestamp_sec) const {
    TransformState state;
    state.pos_x = pos_x.evaluate(timestamp_sec);
    state.pos_y = pos_y.evaluate(timestamp_sec);
    state.scale_x = scale_x.evaluate(timestamp_sec);
    state.scale_y = scale_y.evaluate(timestamp_sec);
    state.rotation_deg = rotation_deg.evaluate(timestamp_sec);
    state.opacity = clampVal(opacity.evaluate(timestamp_sec), 0.0, 1.0);
    state.anchor_x = anchor_x.evaluate(timestamp_sec);
    state.anchor_y = anchor_y.evaluate(timestamp_sec);
    return state;
}

void TransformNode::evaluate_node(double timestamp_sec, RenderPlan& plan) {
    TransformState t = evaluate_transform(timestamp_sec);
    if (!plan.layers.empty()) {
        plan.layers.back().transform = t;
    }
}

EffectNode::EffectNode(std::string id, std::string name)
    : Node(std::move(id), std::move(name), Type::Effect),
      brightness(0.0), contrast(1.0), saturation(1.0), gamma(1.0),
      tint_r(1.0), tint_g(1.0), tint_b(1.0), blur_radius(0.0) {}

EffectState EffectNode::evaluate_effect(double timestamp_sec) const {
    EffectState e;
    e.brightness = clampVal(brightness.evaluate(timestamp_sec), -1.0, 1.0);
    e.contrast = clampVal(contrast.evaluate(timestamp_sec), 0.0, 3.0);
    e.saturation = clampVal(saturation.evaluate(timestamp_sec), 0.0, 3.0);
    e.gamma = clampVal(gamma.evaluate(timestamp_sec), 0.1, 3.0);
    e.tint_r = clampVal(tint_r.evaluate(timestamp_sec), 0.0, 2.0);
    e.tint_g = clampVal(tint_g.evaluate(timestamp_sec), 0.0, 2.0);
    e.tint_b = clampVal(tint_b.evaluate(timestamp_sec), 0.0, 2.0);
    e.blur_radius = clampVal(blur_radius.evaluate(timestamp_sec), 0.0, 50.0);
    e.blend_mode = blend_mode;
    return e;
}

void EffectNode::evaluate_node(double timestamp_sec, RenderPlan& plan) {
    EffectState e = evaluate_effect(timestamp_sec);
    if (!plan.layers.empty()) {
        plan.layers.back().effect = e;
    }
}

CompositeNode::CompositeNode(std::string id, std::string name, int width, int height)
    : Node(std::move(id), std::move(name), Type::Composite), width_(width), height_(height) {}

void CompositeNode::evaluate_node(double timestamp_sec, RenderPlan& plan) {
    (void)timestamp_sec;
    plan.canvas_width = width_;
    plan.canvas_height = height_;
}

// ---------------- RenderGraph Implementation ----------------

RenderGraph::RenderGraph() = default;
RenderGraph::~RenderGraph() = default;

void RenderGraph::add_node(std::shared_ptr<Node> node) {
    if (!node) return;
    nodes_[node->id()] = node;
}

void RenderGraph::remove_node(const std::string& node_id) {
    nodes_.erase(node_id);
    adjacency_list_.erase(node_id);
    in_edges_.erase(node_id);

    for (auto& [_, neighbors] : adjacency_list_) {
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), node_id), neighbors.end());
    }
    for (auto& [_, predecessors] : in_edges_) {
        predecessors.erase(std::remove(predecessors.begin(), predecessors.end(), node_id), predecessors.end());
    }
}

std::shared_ptr<Node> RenderGraph::get_node(const std::string& node_id) const {
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

bool RenderGraph::connect_nodes(const std::string& from_id, const std::string& to_id) {
    if (!nodes_.count(from_id) || !nodes_.count(to_id)) {
        return false;
    }

    adjacency_list_[from_id].push_back(to_id);
    in_edges_[to_id].push_back(from_id);

    // Verify DAG property (no cycles allowed)
    if (has_cycle()) {
        disconnect_nodes(from_id, to_id);
        std::cerr << "[RenderGraph] Cycle detected! Connection rejected: " << from_id << " -> " << to_id << std::endl;
        return false;
    }

    return true;
}

void RenderGraph::disconnect_nodes(const std::string& from_id, const std::string& to_id) {
    if (adjacency_list_.count(from_id)) {
        auto& neighbors = adjacency_list_[from_id];
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), to_id), neighbors.end());
    }
    if (in_edges_.count(to_id)) {
        auto& predecessors = in_edges_[to_id];
        predecessors.erase(std::remove(predecessors.begin(), predecessors.end(), from_id), predecessors.end());
    }
}

void RenderGraph::clear() {
    nodes_.clear();
    adjacency_list_.clear();
    in_edges_.clear();
}

bool RenderGraph::has_cycle() const {
    enum Color { White, Gray, Black };
    std::unordered_map<std::string, Color> color;
    for (const auto& [id, _] : nodes_) {
        color[id] = White;
    }

    std::function<bool(const std::string&)> dfs = [&](const std::string& u) -> bool {
        color[u] = Gray;
        if (adjacency_list_.count(u)) {
            for (const auto& v : adjacency_list_.at(u)) {
                if (color[v] == Gray) return true; // Back edge -> cycle!
                if (color[v] == White && dfs(v)) return true;
            }
        }
        color[u] = Black;
        return false;
    };

    for (const auto& [id, _] : nodes_) {
        if (color[id] == White) {
            if (dfs(id)) return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Node>> RenderGraph::get_topological_order() const {
    std::unordered_map<std::string, int> in_degree;
    for (const auto& [id, _] : nodes_) {
        in_degree[id] = 0;
    }

    for (const auto& [_, neighbors] : adjacency_list_) {
        for (const auto& v : neighbors) {
            in_degree[v]++;
        }
    }

    std::queue<std::string> q;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) {
            q.push(id);
        }
    }

    std::vector<std::shared_ptr<Node>> order;
    while (!q.empty()) {
        std::string u = q.front();
        q.pop();

        if (nodes_.count(u)) {
            order.push_back(nodes_.at(u));
        }

        if (adjacency_list_.count(u)) {
            for (const auto& v : adjacency_list_.at(u)) {
                in_degree[v]--;
                if (in_degree[v] == 0) {
                    q.push(v);
                }
            }
        }
    }

    // If topological sort missed nodes (due to isolated components), add them
    if (order.size() < nodes_.size()) {
        for (const auto& [id, node] : nodes_) {
            if (std::find(order.begin(), order.end(), node) == order.end()) {
                order.push_back(node);
            }
        }
    }

    return order;
}

RenderPlan RenderGraph::evaluate(double timestamp_seconds) {
    RenderPlan plan;
    plan.timestamp_seconds = timestamp_seconds;
    plan.canvas_width = canvas_width_;
    plan.canvas_height = canvas_height_;

    auto ordered_nodes = get_topological_order();
    for (const auto& node : ordered_nodes) {
        if (node) {
            node->evaluate_node(timestamp_seconds, plan);
        }
    }

    return plan;
}

namespace {

inline uint8_t blendComponent(uint8_t base, uint8_t blend, BlendMode mode) {
    double b = base / 255.0;
    double l = blend / 255.0;
    double r = b;

    switch (mode) {
        case BlendMode::Normal:   r = l; break;
        case BlendMode::Add:      r = std::min(1.0, b + l); break;
        case BlendMode::Multiply: r = b * l; break;
        case BlendMode::Screen:   r = 1.0 - (1.0 - b) * (1.0 - l); break;
        case BlendMode::Overlay:
            r = (b < 0.5) ? (2.0 * b * l) : (1.0 - 2.0 * (1.0 - b) * (1.0 - l));
            break;
        case BlendMode::Darken:   r = std::min(b, l); break;
        case BlendMode::Lighten:  r = std::max(b, l); break;
    }
    return static_cast<uint8_t>(clampVal(r * 255.0, 0.0, 255.0));
}

// Generate color test bars pattern for procedural generator clips
void renderTestBars(std::vector<uint8_t>& buf, int w, int h, double t) {
    buf.resize(w * h * 4);
    const uint32_t colors[8] = {
        0xFFE0E0E0, // White
        0xFFD0D000, // Yellow
        0xFF00D0D0, // Cyan
        0xFF00D000, // Green
        0xFFD000D0, // Magenta
        0xFFD00000, // Red
        0xFF0000D0, // Blue
        0xFF101010  // Black
    };

    int bar_width = w / 8;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int bar_idx = std::min(7, x / bar_width);
            uint32_t c = colors[bar_idx];

            // Animated subtle gradient band at bottom
            if (y > (h * 3) / 4) {
                int moving_x = static_cast<int>(x + t * 60.0) % w;
                uint8_t sweep = static_cast<uint8_t>((moving_x * 255) / w);
                c = (0xFF000000) | (sweep << 16) | (sweep << 8) | sweep;
            }

            int idx = (y * w + x) * 4;
            buf[idx + 0] = (c >> 16) & 0xFF; // R
            buf[idx + 1] = (c >> 8) & 0xFF;  // G
            buf[idx + 2] = (c) & 0xFF;       // B
            buf[idx + 3] = (c >> 24) & 0xFF; // A
        }
    }
}

} // namespace

DecodedVideoFrame RenderGraph::render_frame(
    double timestamp_seconds,
    int output_width,
    int output_height,
    const std::function<std::shared_ptr<DecodedVideoFrame>(const std::string&, double)>& frame_fetcher) {

    RenderPlan plan = evaluate(timestamp_seconds);
    int w = output_width > 0 ? output_width : plan.canvas_width;
    int h = output_height > 0 ? output_height : plan.canvas_height;

    DecodedVideoFrame output_frame;
    output_frame.width = w;
    output_frame.height = h;
    output_frame.stride = w * 4;
    output_frame.pts_seconds = timestamp_seconds;
    output_frame.rgba_data.resize(w * h * 4);

    // Fill background (Dark slate / grid)
    uint8_t bg_r = (plan.background_color >> 16) & 0xFF;
    uint8_t bg_g = (plan.background_color >> 8) & 0xFF;
    uint8_t bg_b = (plan.background_color) & 0xFF;
    uint8_t bg_a = (plan.background_color >> 24) & 0xFF;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 4;
            // Subtle editor grid pattern
            bool grid = (x % 64 == 0) || (y % 64 == 0);
            output_frame.rgba_data[idx + 0] = grid ? static_cast<uint8_t>(std::min(255, bg_r + 15)) : bg_r;
            output_frame.rgba_data[idx + 1] = grid ? static_cast<uint8_t>(std::min(255, bg_g + 15)) : bg_g;
            output_frame.rgba_data[idx + 2] = grid ? static_cast<uint8_t>(std::min(255, bg_b + 18)) : bg_b;
            output_frame.rgba_data[idx + 3] = bg_a;
        }
    }

    // Composite active layers
    for (const auto& layer : plan.layers) {
        if (!layer.is_visible) continue;

        std::shared_ptr<DecodedVideoFrame> src_frame = nullptr;
        std::vector<uint8_t> gen_buffer;

        if (layer.clip.is_generator) {
            renderTestBars(gen_buffer, w, h, timestamp_seconds);
        } else if (frame_fetcher) {
            src_frame = frame_fetcher(layer.clip.media_path, layer.source_time_offset);
        }

        const uint8_t* src_data = nullptr;
        int src_w = w;
        int src_h = h;

        if (src_frame && !src_frame->rgba_data.empty()) {
            src_data = src_frame->rgba_data.data();
            src_w = src_frame->width;
            src_h = src_frame->height;
        } else if (!gen_buffer.empty()) {
            src_data = gen_buffer.data();
            src_w = w;
            src_h = h;
        }

        if (!src_data) continue;

        const auto& t = layer.transform;
        const auto& e = layer.effect;

        double scale_x = (std::abs(t.scale_x) > 1e-4) ? t.scale_x : 1.0;
        double scale_y = (std::abs(t.scale_y) > 1e-4) ? t.scale_y : 1.0;
        double rad = t.rotation_deg * (M_PI / 180.0);
        double cos_r = std::cos(-rad);
        double sin_r = std::sin(-rad);

        double center_x = w * 0.5 + t.pos_x;
        double center_y = h * 0.5 + t.pos_y;

        double inv_sx = 1.0 / scale_x;
        double inv_sy = 1.0 / scale_y;

        // Bounding rect rasterization
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Map output canvas (x, y) to layer source space (u, v)
                double dx = x - center_x;
                double dy = y - center_y;

                // Inverse rotation
                double rx = dx * cos_r - dy * sin_r;
                double ry = dx * sin_r + dy * cos_r;

                // Inverse scale & anchor offset
                double src_x = (rx * inv_sx) + (src_w * t.anchor_x);
                double src_y = (ry * inv_sy) + (src_h * t.anchor_y);

                int ix = static_cast<int>(std::floor(src_x));
                int iy = static_cast<int>(std::floor(src_y));

                if (ix < 0 || ix >= src_w || iy < 0 || iy >= src_h) {
                    continue; // Outside layer bounding box
                }

                int src_idx = (iy * src_w + ix) * 4;
                uint8_t sr = src_data[src_idx + 0];
                uint8_t sg = src_data[src_idx + 1];
                uint8_t sb = src_data[src_idx + 2];
                uint8_t sa = src_data[src_idx + 3];

                // Apply Color Grading / Effects
                double r_f = sr / 255.0;
                double g_f = sg / 255.0;
                double b_f = sb / 255.0;

                // Brightness & Contrast
                r_f = (r_f - 0.5) * e.contrast + 0.5 + e.brightness;
                g_f = (g_f - 0.5) * e.contrast + 0.5 + e.brightness;
                b_f = (b_f - 0.5) * e.contrast + 0.5 + e.brightness;

                // Saturation
                double luma = 0.2126 * r_f + 0.7152 * g_f + 0.0722 * b_f;
                r_f = luma + e.saturation * (r_f - luma);
                g_f = luma + e.saturation * (g_f - luma);
                b_f = luma + e.saturation * (b_f - luma);

                // Tint
                r_f *= e.tint_r;
                g_f *= e.tint_g;
                b_f *= e.tint_b;

                // Gamma
                if (e.gamma != 1.0 && e.gamma > 0.01) {
                    double inv_gamma = 1.0 / e.gamma;
                    r_f = std::pow(clampVal(r_f, 0.0, 1.0), inv_gamma);
                    g_f = std::pow(clampVal(g_f, 0.0, 1.0), inv_gamma);
                    b_f = std::pow(clampVal(b_f, 0.0, 1.0), inv_gamma);
                }

                uint8_t adj_r = static_cast<uint8_t>(clampVal(r_f * 255.0, 0.0, 255.0));
                uint8_t adj_g = static_cast<uint8_t>(clampVal(g_f * 255.0, 0.0, 255.0));
                uint8_t adj_b = static_cast<uint8_t>(clampVal(b_f * 255.0, 0.0, 255.0));

                double final_alpha = (sa / 255.0) * t.opacity;

                int dst_idx = (y * w + x) * 4;
                uint8_t dst_r = output_frame.rgba_data[dst_idx + 0];
                uint8_t dst_g = output_frame.rgba_data[dst_idx + 1];
                uint8_t dst_b = output_frame.rgba_data[dst_idx + 2];

                // Blend mode application
                uint8_t blended_r = blendComponent(dst_r, adj_r, e.blend_mode);
                uint8_t blended_g = blendComponent(dst_g, adj_g, e.blend_mode);
                uint8_t blended_b = blendComponent(dst_b, adj_b, e.blend_mode);

                // Alpha compositing (Porter-Duff Over)
                output_frame.rgba_data[dst_idx + 0] = static_cast<uint8_t>(dst_r * (1.0 - final_alpha) + blended_r * final_alpha);
                output_frame.rgba_data[dst_idx + 1] = static_cast<uint8_t>(dst_g * (1.0 - final_alpha) + blended_g * final_alpha);
                output_frame.rgba_data[dst_idx + 2] = static_cast<uint8_t>(dst_b * (1.0 - final_alpha) + blended_b * final_alpha);
                output_frame.rgba_data[dst_idx + 3] = 255;
            }
        }
    }

    return output_frame;
}

void RenderGraph::setup_default_preset(const std::string& media_path) {
    clear();

    // Node 1: Video / Media Clip
    ClipState clip;
    clip.clip_id = "clip_main";
    clip.media_path = media_path;
    clip.start_time = 0.0;
    clip.duration = 300.0;
    clip.in_point = 0.0;
    clip.is_generator = media_path.empty();
    auto clip_node = std::make_shared<ClipNode>("clip_1", "Main Video Stream", clip);
    add_node(clip_node);

    // Node 2: 2D Transform Node with smooth animated Bézier keyframes
    auto transform_node = std::make_shared<TransformNode>("transform_1", "Motion & Scaling");
    transform_node->scale_x.add_keyframe(0.0, 1.0, EasingType::EaseInOut);
    transform_node->scale_x.add_keyframe(5.0, 1.05, EasingType::EaseInOut);
    transform_node->scale_y.add_keyframe(0.0, 1.0, EasingType::EaseInOut);
    transform_node->scale_y.add_keyframe(5.0, 1.05, EasingType::EaseInOut);
    transform_node->opacity.set_default_value(1.0);
    add_node(transform_node);

    // Node 3: Color Grading & Effect Node
    auto effect_node = std::make_shared<EffectNode>("effect_1", "Color Grade & Look");
    effect_node->contrast.set_default_value(1.05);
    effect_node->saturation.set_default_value(1.10);
    effect_node->brightness.set_default_value(0.0);
    add_node(effect_node);

    // Node 4: Output Composite Node
    auto composite_node = std::make_shared<CompositeNode>("out_1", "Canvas Compositor", 1920, 1080);
    add_node(composite_node);

    // Connect DAG Pipeline: Clip -> Transform -> Effect -> Composite
    connect_nodes("clip_1", "transform_1");
    connect_nodes("transform_1", "effect_1");
    connect_nodes("effect_1", "out_1");
}

} // namespace antigravity::core
