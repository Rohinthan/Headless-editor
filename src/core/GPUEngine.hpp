#pragma once

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include "GraphEngine.hpp"
#include "DecoderEngine.hpp"
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <GL/gl.h>
#include <GL/glext.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace antigravity::core {

// 4x4 Matrix Mathematics for GPU Transformation Pipelines
struct Mat4 {
    float m[16];

    Mat4();
    static Mat4 identity();
    static Mat4 ortho(float left, float right, float bottom, float top, float near_val = -1.0f, float far_val = 1.0f);
    static Mat4 translate(float x, float y, float z = 0.0f);
    static Mat4 scale(float sx, float sy, float sz = 1.0f);
    static Mat4 rotateZ(float angle_degrees);

    Mat4 operator*(const Mat4& o) const;
    const float* data() const { return m; }

    // Computes M = T(x, y) * R(theta) * S(sx, sy) * T(-ax * w, -ay * h)
    static Mat4 computeLayerTransform(
        float pos_x, float pos_y,
        float scale_x, float scale_y,
        float rot_deg,
        float anchor_x, float anchor_y,
        float layer_w, float layer_h
    );
};

// RAII OpenGL Texture
class GLTexture {
public:
    GLTexture();
    ~GLTexture();

    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    void create(int width, int height, const uint8_t* rgba_data = nullptr);
    void update(const uint8_t* rgba_data, int width = -1, int height = -1);
    void bind(unsigned int unit = 0) const;
    void unbind() const;

    unsigned int id() const { return texture_id_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool is_valid() const { return texture_id_ != 0; }

private:
    unsigned int texture_id_ = 0;
    int width_ = 0;
    int height_ = 0;
};

// RAII OpenGL Framebuffer Object with attached Color Texture
class GLFramebuffer {
public:
    GLFramebuffer();
    ~GLFramebuffer();

    GLFramebuffer(const GLFramebuffer&) = delete;
    GLFramebuffer& operator=(const GLFramebuffer&) = delete;
    GLFramebuffer(GLFramebuffer&& other) noexcept;
    GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

    bool create(int width, int height);
    void bind();
    void unbind();
    void resize(int width, int height);

    unsigned int fbo_id() const { return fbo_id_; }
    const GLTexture& texture() const { return color_texture_; }
    GLTexture& texture() { return color_texture_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool is_valid() const { return fbo_id_ != 0; }

private:
    unsigned int fbo_id_ = 0;
    GLTexture color_texture_;
    int width_ = 0;
    int height_ = 0;
};

// RAII GLSL Shader Program
class GLShaderProgram {
public:
    GLShaderProgram();
    ~GLShaderProgram();

    bool compileAndLink(const std::string& vertex_src, const std::string& fragment_src);
    void use() const;
    void unuse() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat4(const std::string& name, const Mat4& matrix);

    unsigned int id() const { return program_id_; }
    bool is_valid() const { return program_id_ != 0; }
    std::string getLog() const { return log_; }

private:
    int getUniformLocation(const std::string& name);
    unsigned int compileShader(unsigned int type, const std::string& source);

    unsigned int program_id_ = 0;
    std::unordered_map<std::string, int> uniform_cache_;
    std::string log_;
};

// Multi-Layer GPU Compositing Engine
class GPUEngine {
public:
    GPUEngine();
    ~GPUEngine();

    // Offscreen context management for headless CLI & worker threads
    bool initContext(int width = 1920, int height = 1080);
    void makeCurrent();
    void doneCurrent();

    // Shader compilation
    bool initShaders();

    // Set canvas dimensions
    void setCanvasResolution(int width, int height);
    int canvasWidth() const { return canvas_width_; }
    int canvasHeight() const { return canvas_height_; }

    // Upload / cache layer video textures from CPU/Decoder
    std::shared_ptr<GLTexture> getOrCreateLayerTexture(const std::string& key, int width, int height, const uint8_t* rgba_data = nullptr);

    // Multi-layer GPU ping-pong compositing pass
    // Evaluates the RenderPlan and renders the composite into the output FBO
    void renderComposite(
        const RenderPlan& plan,
        const std::function<std::shared_ptr<DecodedVideoFrame>(const std::string&, double)>& frame_fetcher = nullptr
    );

    // Read back final composite from GPU VRAM into DecodedVideoFrame (host RAM)
    DecodedVideoFrame readbackOutputFrame(double timestamp_seconds);

    // Get the output texture ID (for direct QSG Scene Graph sharing)
    unsigned int getOutputTextureId() const;

    // Direct access to shaders for inspection/diagnostics
    const GLShaderProgram& blendShader() const { return blend_shader_; }
    const GLShaderProgram& colorGradeShader() const { return color_grade_shader_; }

    // GPU System Information
    std::string getRendererString() const;
    std::string getOpenGLVersionString() const;
    std::string getGLSLVersionString() const;

private:
    void initQuadVAO();
    void renderQuad();

    std::unique_ptr<QOpenGLContext> gl_context_;
    std::unique_ptr<QOffscreenSurface> offscreen_surface_;
    bool owns_context_ = false;

    int canvas_width_ = 1920;
    int canvas_height_ = 1080;

    // FBO Ping-Pong Pipeline
    GLFramebuffer fbo_ping_;
    GLFramebuffer fbo_pong_;
    GLFramebuffer fbo_layer_grade_; // Intermediate color-graded layer target

    // Shader Programs
    GLShaderProgram blend_shader_;
    GLShaderProgram color_grade_shader_;

    // Screen-aligned Quad geometry (VAO / VBO)
    unsigned int quad_vao_ = 0;
    unsigned int quad_vbo_ = 0;

    // Texture cache
    std::unordered_map<std::string, std::shared_ptr<GLTexture>> texture_cache_;
    std::shared_ptr<GLTexture> test_pattern_texture_;
};

} // namespace antigravity::core
