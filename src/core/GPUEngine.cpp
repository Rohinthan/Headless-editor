#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include "GPUEngine.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>

namespace antigravity::core {

// Embedded Shader Fallbacks for standalone builds
namespace {

const char* EMBEDDED_COMPOSITOR_VERT = R"(#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

uniform mat4 u_projection;
uniform mat4 u_transform;

out vec2 v_texCoord;

void main() {
    v_texCoord = a_texCoord;
    gl_Position = u_projection * u_transform * vec4(a_position, 0.0, 1.0);
}
)";

const char* EMBEDDED_BLEND_MODES_FRAG = R"(#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_baseTexture;
uniform sampler2D u_layerTexture;
uniform int u_blendMode;
uniform float u_opacity;

vec3 blendAdd(vec3 base, vec3 blend) {
    return min(vec3(1.0), base + blend);
}
vec3 blendMultiply(vec3 base, vec3 blend) {
    return base * blend;
}
vec3 blendScreen(vec3 base, vec3 blend) {
    return vec3(1.0) - (vec3(1.0) - base) * (vec3(1.0) - blend);
}
float blendOverlayChannel(float b, float l) {
    return (b < 0.5) ? (2.0 * b * l) : (1.0 - 2.0 * (1.0 - b) * (1.0 - l));
}
vec3 blendOverlay(vec3 base, vec3 blend) {
    return vec3(
        blendOverlayChannel(base.r, blend.r),
        blendOverlayChannel(base.g, blend.g),
        blendOverlayChannel(base.b, blend.b)
    );
}
float blendSoftLightChannel(float b, float l) {
    return (l < 0.5)
        ? (2.0 * b * l + b * b * (1.0 - 2.0 * l))
        : (sqrt(b) * (2.0 * l - 1.0) + 2.0 * b * (1.0 - l));
}
vec3 blendSoftLight(vec3 base, vec3 blend) {
    return vec3(
        blendSoftLightChannel(base.r, blend.r),
        blendSoftLightChannel(base.g, blend.g),
        blendSoftLightChannel(base.b, blend.b)
    );
}
float blendColorDodgeChannel(float b, float l) {
    return (l >= 1.0) ? 1.0 : min(1.0, b / (1.0 - l));
}
vec3 blendColorDodge(vec3 base, vec3 blend) {
    return vec3(
        blendColorDodgeChannel(base.r, blend.r),
        blendColorDodgeChannel(base.g, blend.g),
        blendColorDodgeChannel(base.b, blend.b)
    );
}

void main() {
    vec4 baseColor = texture(u_baseTexture, v_texCoord);
    vec4 layerColor = texture(u_layerTexture, v_texCoord);

    float alpha = layerColor.a * clamp(u_opacity, 0.0, 1.0);
    if (alpha <= 0.0) {
        fragColor = baseColor;
        return;
    }

    vec3 blendedRGB = layerColor.rgb;
    if (u_blendMode == 1) {
        blendedRGB = blendAdd(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 2) {
        blendedRGB = blendMultiply(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 3) {
        blendedRGB = blendScreen(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 4) {
        blendedRGB = blendOverlay(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 5) {
        blendedRGB = blendSoftLight(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 6) {
        blendedRGB = blendColorDodge(baseColor.rgb, layerColor.rgb);
    }

    vec3 outRGB = mix(baseColor.rgb, blendedRGB, alpha);
    float outAlpha = clamp(baseColor.a + alpha * (1.0 - baseColor.a), 0.0, 1.0);
    fragColor = vec4(outRGB, outAlpha);
}
)";

const char* EMBEDDED_COLOR_GRADE_FRAG = R"(#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_exposure;
uniform float u_brightness;
uniform float u_contrast;
uniform float u_saturation;
uniform float u_temperature;
uniform float u_tint;
uniform vec3 u_lift;
uniform vec3 u_gamma_rgb;
uniform vec3 u_gain;

const vec3 LUMA_REC709 = vec3(0.2126, 0.7152, 0.0722);

void main() {
    vec4 src = texture(u_inputTexture, v_texCoord);
    vec3 rgb = src.rgb;

    if (abs(u_exposure) > 0.001) {
        rgb *= exp2(u_exposure);
    }

    if (abs(u_temperature) > 0.001 || abs(u_tint) > 0.001) {
        vec3 tempShift = vec3(
            u_temperature * 0.15 + u_tint * 0.10,
            -u_tint * 0.15,
            -u_temperature * 0.15 + u_tint * 0.05
        );
        rgb = clamp(rgb + tempShift, 0.0, 1.0);
    }

    vec3 liftFactor = u_lift * (vec3(1.0) - rgb);
    vec3 gainFactor = u_gain * rgb;
    rgb = clamp(liftFactor + gainFactor, 0.0, 1.0);

    vec3 invGammaRGB = vec3(1.0) / max(u_gamma_rgb, vec3(0.01));
    rgb = pow(rgb, invGammaRGB);

    rgb = (rgb - vec3(0.5)) * u_contrast + vec3(0.5) + vec3(u_brightness);
    rgb = clamp(rgb, 0.0, 1.0);

    float luma = dot(rgb, LUMA_REC709);
    rgb = mix(vec3(luma), rgb, u_saturation);
    rgb = clamp(rgb, 0.0, 1.0);

    fragColor = vec4(rgb, src.a);
}
)";

std::string loadShaderSource(const std::string& filepath, const char* fallback) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return std::string(fallback);
}

} // namespace

// ==================== Mat4 Implementation ====================

Mat4::Mat4() {
    std::memset(m, 0, sizeof(m));
}

Mat4 Mat4::identity() {
    Mat4 res;
    res.m[0]  = 1.0f; res.m[5]  = 1.0f;
    res.m[10] = 1.0f; res.m[15] = 1.0f;
    return res;
}

Mat4 Mat4::ortho(float left, float right, float bottom, float top, float near_val, float far_val) {
    Mat4 res;
    res.m[0]  = 2.0f / (right - left);
    res.m[5]  = 2.0f / (top - bottom);
    res.m[10] = -2.0f / (far_val - near_val);
    res.m[12] = -(right + left) / (right - left);
    res.m[13] = -(top + bottom) / (top - bottom);
    res.m[14] = -(far_val + near_val) / (far_val - near_val);
    res.m[15] = 1.0f;
    return res;
}

Mat4 Mat4::translate(float x, float y, float z) {
    Mat4 res = identity();
    res.m[12] = x;
    res.m[13] = y;
    res.m[14] = z;
    return res;
}

Mat4 Mat4::scale(float sx, float sy, float sz) {
    Mat4 res = identity();
    res.m[0]  = sx;
    res.m[5]  = sy;
    res.m[10] = sz;
    return res;
}

Mat4 Mat4::rotateZ(float angle_degrees) {
    Mat4 res = identity();
    float rad = angle_degrees * (float(M_PI) / 180.0f);
    float c = std::cos(rad);
    float s = std::sin(rad);

    res.m[0] = c;  res.m[1] = s;
    res.m[4] = -s; res.m[5] = c;
    return res;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 res;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += m[k * 4 + row] * o.m[col * 4 + k];
            }
            res.m[col * 4 + row] = sum;
        }
    }
    return res;
}

// Compute: M = T(pos) * R(rot) * S(scale) * T(-anchor * size)
Mat4 Mat4::computeLayerTransform(
    float pos_x, float pos_y,
    float scale_x, float scale_y,
    float rot_deg,
    float anchor_x, float anchor_y,
    float layer_w, float layer_h) {

    // Anchor offset: align relative to anchor point
    float ax = (anchor_x - 0.5f) * layer_w;
    float ay = (anchor_y - 0.5f) * layer_h;

    Mat4 t_anchor_neg = Mat4::translate(-ax, -ay, 0.0f);
    Mat4 s = Mat4::scale(scale_x * (layer_w * 0.5f), scale_y * (layer_h * 0.5f), 1.0f);
    Mat4 r = Mat4::rotateZ(rot_deg);
    Mat4 t_pos = Mat4::translate(pos_x, pos_y, 0.0f);

    return t_pos * r * s * t_anchor_neg;
}

// ==================== GLTexture Implementation ====================

GLTexture::GLTexture() = default;

GLTexture::~GLTexture() {
    if (texture_id_) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
}

GLTexture::GLTexture(GLTexture&& other) noexcept {
    *this = std::move(other);
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
    if (this != &other) {
        if (texture_id_) glDeleteTextures(1, &texture_id_);
        texture_id_ = other.texture_id_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void GLTexture::create(int width, int height, const uint8_t* rgba_data) {
    if (!texture_id_) {
        glGenTextures(1, &texture_id_);
    }
    width_ = width;
    height_ = height;

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, rgba_data
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::update(const uint8_t* rgba_data, int width, int height) {
    if (!texture_id_ || (width > 0 && (width != width_ || height != height_))) {
        create(width > 0 ? width : width_, height > 0 ? height : height_, rgba_data);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0,
        width_, height_,
        GL_RGBA, GL_UNSIGNED_BYTE, rgba_data
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void GLTexture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ==================== GLFramebuffer Implementation ====================

GLFramebuffer::GLFramebuffer() = default;

GLFramebuffer::~GLFramebuffer() {
    if (fbo_id_) {
        glDeleteFramebuffers(1, &fbo_id_);
        fbo_id_ = 0;
    }
}

GLFramebuffer::GLFramebuffer(GLFramebuffer&& other) noexcept {
    *this = std::move(other);
}

GLFramebuffer& GLFramebuffer::operator=(GLFramebuffer&& other) noexcept {
    if (this != &other) {
        if (fbo_id_) glDeleteFramebuffers(1, &fbo_id_);
        fbo_id_ = other.fbo_id_;
        color_texture_ = std::move(other.color_texture_);
        width_ = other.width_;
        height_ = other.height_;
        other.fbo_id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

bool GLFramebuffer::create(int width, int height) {
    if (!fbo_id_) {
        glGenFramebuffers(1, &fbo_id_);
    }
    width_ = width;
    height_ = height;

    color_texture_.create(width, height, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, color_texture_.id(), 0
    );

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return (status == GL_FRAMEBUFFER_COMPLETE);
}

void GLFramebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glViewport(0, 0, width_, height_);
}

void GLFramebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::resize(int width, int height) {
    if (width_ == width && height_ == height && fbo_id_ != 0) return;
    create(width, height);
}

// ==================== GLShaderProgram Implementation ====================

GLShaderProgram::GLShaderProgram() = default;

GLShaderProgram::~GLShaderProgram() {
    if (program_id_) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
}

unsigned int GLShaderProgram::compileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        log_ += "[Shader Compile Error]: " + std::string(infoLog) + "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool GLShaderProgram::compileAndLink(const std::string& vertex_src, const std::string& fragment_src) {
    log_.clear();
    uniform_cache_.clear();

    if (program_id_) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertex_src);
    if (!vs) return false;

    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragment_src);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vs);
    glAttachShader(program_id_, fs);
    glLinkProgram(program_id_);

    int success = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program_id_, sizeof(infoLog), nullptr, infoLog);
        log_ += "[Program Link Error]: " + std::string(infoLog) + "\n";
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(program_id_);
        program_id_ = 0;
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

void GLShaderProgram::use() const {
    glUseProgram(program_id_);
}

void GLShaderProgram::unuse() const {
    glUseProgram(0);
}

int GLShaderProgram::getUniformLocation(const std::string& name) {
    auto it = uniform_cache_.find(name);
    if (it != uniform_cache_.end()) {
        return it->second;
    }
    int loc = glGetUniformLocation(program_id_, name.c_str());
    uniform_cache_[name] = loc;
    return loc;
}

void GLShaderProgram::setInt(const std::string& name, int value) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void GLShaderProgram::setFloat(const std::string& name, float value) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void GLShaderProgram::setVec2(const std::string& name, float x, float y) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

void GLShaderProgram::setVec3(const std::string& name, float x, float y, float z) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void GLShaderProgram::setVec4(const std::string& name, float x, float y, float z, float w) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}

void GLShaderProgram::setMat4(const std::string& name, const Mat4& matrix) {
    int loc = getUniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, matrix.data());
}

// ==================== GPUEngine Implementation ====================

GPUEngine::GPUEngine() = default;

GPUEngine::~GPUEngine() {
    if (quad_vao_) glDeleteVertexArrays(1, &quad_vao_);
    if (quad_vbo_) glDeleteBuffers(1, &quad_vbo_);
    if (owns_context_ && gl_context_) {
        doneCurrent();
    }
}

bool GPUEngine::initContext(int width, int height) {
    canvas_width_ = width;
    canvas_height_ = height;

    gl_context_ = std::make_unique<QOpenGLContext>();
    if (!gl_context_->create()) {
        std::cerr << "[GPUEngine] Failed to create QOpenGLContext." << std::endl;
        return false;
    }

    offscreen_surface_ = std::make_unique<QOffscreenSurface>();
    offscreen_surface_->create();

    if (!gl_context_->makeCurrent(offscreen_surface_.get())) {
        std::cerr << "[GPUEngine] Failed makeCurrent on offscreen surface." << std::endl;
        return false;
    }
    owns_context_ = true;

    initQuadVAO();
    if (!initShaders()) {
        std::cerr << "[GPUEngine] Failed to initialize shaders." << std::endl;
        return false;
    }

    fbo_ping_.create(canvas_width_, canvas_height_);
    fbo_pong_.create(canvas_width_, canvas_height_);
    fbo_layer_grade_.create(canvas_width_, canvas_height_);

    return true;
}

void GPUEngine::makeCurrent() {
    if (gl_context_ && offscreen_surface_) {
        gl_context_->makeCurrent(offscreen_surface_.get());
    }
}

void GPUEngine::doneCurrent() {
    if (gl_context_) {
        gl_context_->doneCurrent();
    }
}

void GPUEngine::initQuadVAO() {
    // 2D Quad geometry: X, Y, U, V
    float quadVertices[] = {
        // Position   // UV
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);

    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // UV attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void GPUEngine::renderQuad() {
    glBindVertexArray(quad_vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

bool GPUEngine::initShaders() {
    std::string base_dir = "";
#ifdef SRC_DIR
    base_dir = std::string(SRC_DIR) + "/src/shaders/";
#endif

    std::string vert_src = loadShaderSource(base_dir + "compositor.vert", EMBEDDED_COMPOSITOR_VERT);
    std::string blend_src = loadShaderSource(base_dir + "blend_modes.frag", EMBEDDED_BLEND_MODES_FRAG);
    std::string grade_src = loadShaderSource(base_dir + "color_grade.frag", EMBEDDED_COLOR_GRADE_FRAG);

    if (!blend_shader_.compileAndLink(vert_src, blend_src)) {
        std::cerr << "[GPUEngine] Blend Shader Compilation Failed:\n" << blend_shader_.getLog() << std::endl;
        return false;
    }

    if (!color_grade_shader_.compileAndLink(vert_src, grade_src)) {
        std::cerr << "[GPUEngine] Color Grade Shader Compilation Failed:\n" << color_grade_shader_.getLog() << std::endl;
        return false;
    }

    return true;
}

void GPUEngine::setCanvasResolution(int width, int height) {
    if (canvas_width_ == width && canvas_height_ == height) return;
    canvas_width_ = width;
    canvas_height_ = height;
    fbo_ping_.resize(width, height);
    fbo_pong_.resize(width, height);
    fbo_layer_grade_.resize(width, height);
}

std::shared_ptr<GLTexture> GPUEngine::getOrCreateLayerTexture(const std::string& key, int width, int height, const uint8_t* rgba_data) {
    auto it = texture_cache_.find(key);
    if (it != texture_cache_.end()) {
        if (rgba_data) {
            it->second->update(rgba_data, width, height);
        }
        return it->second;
    }

    auto tex = std::make_shared<GLTexture>();
    tex->create(width, height, rgba_data);
    texture_cache_[key] = tex;
    return tex;
}

std::string GPUEngine::getRendererString() const {
    const char* s = (const char*)glGetString(GL_RENDERER);
    return s ? std::string(s) : "Unknown GPU";
}

std::string GPUEngine::getOpenGLVersionString() const {
    const char* s = (const char*)glGetString(GL_VERSION);
    return s ? std::string(s) : "Unknown GL";
}

std::string GPUEngine::getGLSLVersionString() const {
    const char* s = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    return s ? std::string(s) : "Unknown GLSL";
}

void GPUEngine::renderComposite(
    const RenderPlan& plan,
    const std::function<std::shared_ptr<DecodedVideoFrame>(const std::string&, double)>& frame_fetcher) {

    setCanvasResolution(plan.canvas_width, plan.canvas_height);

    // Ping-pong pointers
    GLFramebuffer* base_fbo = &fbo_ping_;
    GLFramebuffer* dest_fbo = &fbo_pong_;

    // Step 0: Clear base FBO with background color
    base_fbo->bind();
    float bg_r = ((plan.background_color >> 16) & 0xFF) / 255.0f;
    float bg_g = ((plan.background_color >> 8) & 0xFF) / 255.0f;
    float bg_b = ((plan.background_color) & 0xFF) / 255.0f;
    float bg_a = ((plan.background_color >> 24) & 0xFF) / 255.0f;

    glClearColor(bg_r, bg_g, bg_b, bg_a);
    glClear(GL_COLOR_BUFFER_BIT);
    base_fbo->unbind();

    Mat4 ortho_proj = Mat4::ortho(
        -canvas_width_ * 0.5f,  canvas_width_ * 0.5f,
        -canvas_height_ * 0.5f, canvas_height_ * 0.5f,
        -1.0f, 1.0f
    );
    Mat4 identity_mat = Mat4::identity();

    // Iterate through active layers in topological order
    for (const auto& layer : plan.layers) {
        if (!layer.is_visible) continue;

        std::shared_ptr<GLTexture> layer_tex = nullptr;

        if (layer.clip.is_generator) {
            if (!test_pattern_texture_) {
                // Generate 8-color test bars texture
                int w = canvas_width_, h = canvas_height_;
                std::vector<uint8_t> test_bars(w * h * 4);
                const uint32_t colors[8] = {
                    0xFFE0E0E0, 0xFFD0D000, 0xFF00D0D0, 0xFF00D000,
                    0xFFD000D0, 0xFFD00000, 0xFF0000D0, 0xFF101010
                };
                int bar_w = w / 8;
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        uint32_t c = colors[std::min(7, x / bar_w)];
                        int idx = (y * w + x) * 4;
                        test_bars[idx + 0] = (c >> 16) & 0xFF;
                        test_bars[idx + 1] = (c >> 8) & 0xFF;
                        test_bars[idx + 2] = (c) & 0xFF;
                        test_bars[idx + 3] = 0xFF;
                    }
                }
                test_pattern_texture_ = std::make_shared<GLTexture>();
                test_pattern_texture_->create(w, h, test_bars.data());
            }
            layer_tex = test_pattern_texture_;
        } else if (frame_fetcher) {
            auto decoded_frame = frame_fetcher(layer.clip.media_path, layer.source_time_offset);
            if (decoded_frame && !decoded_frame->rgba_data.empty()) {
                layer_tex = getOrCreateLayerTexture(
                    layer.clip.clip_id,
                    decoded_frame->width,
                    decoded_frame->height,
                    decoded_frame->rgba_data.data()
                );
            }
        }

        if (!layer_tex || !layer_tex->is_valid()) continue;

        const auto& t = layer.transform;
        const auto& e = layer.effect;
        float layer_w = static_cast<float>(layer_tex->width());
        float layer_h = static_cast<float>(layer_tex->height());

        // Pass A: Color Grading (if color controls are active)
        GLTexture* input_to_blend = layer_tex.get();
        bool needs_grading = (
            std::abs(e.brightness) > 1e-4 ||
            std::abs(e.contrast - 1.0) > 1e-4 ||
            std::abs(e.saturation - 1.0) > 1e-4 ||
            std::abs(e.gamma - 1.0) > 1e-4 ||
            std::abs(e.tint_r - 1.0) > 1e-4 ||
            std::abs(e.tint_g - 1.0) > 1e-4 ||
            std::abs(e.tint_b - 1.0) > 1e-4
        );

        if (needs_grading) {
            fbo_layer_grade_.resize(layer_tex->width(), layer_tex->height());
            fbo_layer_grade_.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            color_grade_shader_.use();
            color_grade_shader_.setMat4("u_projection", identity_mat);
            color_grade_shader_.setMat4("u_transform", identity_mat);
            color_grade_shader_.setFloat("u_exposure", 0.0f);
            color_grade_shader_.setFloat("u_brightness", static_cast<float>(e.brightness));
            color_grade_shader_.setFloat("u_contrast", static_cast<float>(e.contrast));
            color_grade_shader_.setFloat("u_saturation", static_cast<float>(e.saturation));
            color_grade_shader_.setFloat("u_temperature", 0.0f);
            color_grade_shader_.setFloat("u_tint", 0.0f);
            color_grade_shader_.setVec3("u_lift", 0.0f, 0.0f, 0.0f);
            color_grade_shader_.setVec3("u_gamma_rgb", static_cast<float>(e.gamma), static_cast<float>(e.gamma), static_cast<float>(e.gamma));
            color_grade_shader_.setVec3("u_gain", static_cast<float>(e.tint_r), static_cast<float>(e.tint_g), static_cast<float>(e.tint_b));

            layer_tex->bind(0);
            color_grade_shader_.setInt("u_inputTexture", 0);

            renderQuad();
            fbo_layer_grade_.unbind();
            color_grade_shader_.unuse();

            input_to_blend = &fbo_layer_grade_.texture();
        }

        // Pass B: Blend into Destination Ping-Pong FBO
        dest_fbo->bind();
        blend_shader_.use();

        Mat4 layer_transform = Mat4::computeLayerTransform(
            static_cast<float>(t.pos_x),
            static_cast<float>(t.pos_y),
            static_cast<float>(t.scale_x),
            static_cast<float>(t.scale_y),
            static_cast<float>(t.rotation_deg),
            static_cast<float>(t.anchor_x),
            static_cast<float>(t.anchor_y),
            layer_w,
            layer_h
        );

        blend_shader_.setMat4("u_projection", ortho_proj);
        blend_shader_.setMat4("u_transform", layer_transform);
        blend_shader_.setInt("u_blendMode", static_cast<int>(e.blend_mode));
        blend_shader_.setFloat("u_opacity", static_cast<float>(t.opacity));

        base_fbo->texture().bind(0);
        blend_shader_.setInt("u_baseTexture", 0);

        input_to_blend->bind(1);
        blend_shader_.setInt("u_layerTexture", 1);

        renderQuad();

        dest_fbo->unbind();
        blend_shader_.unuse();

        // Swap ping-pong FBOs
        std::swap(base_fbo, dest_fbo);
    }
}

DecodedVideoFrame GPUEngine::readbackOutputFrame(double timestamp_seconds) {
    DecodedVideoFrame frame;
    frame.width = canvas_width_;
    frame.height = canvas_height_;
    frame.stride = canvas_width_ * 4;
    frame.pts_seconds = timestamp_seconds;
    frame.rgba_data.resize(frame.stride * canvas_height_);

    fbo_ping_.bind();
    glReadPixels(
        0, 0,
        canvas_width_, canvas_height_,
        GL_RGBA, GL_UNSIGNED_BYTE,
        frame.rgba_data.data()
    );
    fbo_ping_.unbind();

    // Flip vertically for standard top-left image orientation
    int stride = frame.stride;
    std::vector<uint8_t> row_temp(stride);
    for (int y = 0; y < canvas_height_ / 2; ++y) {
        uint8_t* row_top = frame.rgba_data.data() + y * stride;
        uint8_t* row_bot = frame.rgba_data.data() + (canvas_height_ - 1 - y) * stride;
        std::memcpy(row_temp.data(), row_top, stride);
        std::memcpy(row_top, row_bot, stride);
        std::memcpy(row_bot, row_temp.data(), stride);
    }

    return frame;
}

unsigned int GPUEngine::getOutputTextureId() const {
    return fbo_ping_.texture().id();
}

} // namespace antigravity::core
