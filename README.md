# Headless-Editor ⚡

A high-performance, lightweight Linux Video Editor core and GPU Compositing Engine implemented in **C++20**, **OpenGL / GLSL**, **Qt 6 (QML)**, and **FFmpeg**. Built for hardware-accelerated video decoding (VA-API / CUDA), multi-layer GPU ping-pong compositing, 4x4 matrix affine transformations, cubic Bézier keyframe animation, and real-time preview viewport rendering.

---

## ✨ Features

- **🚀 Hardware-Accelerated Decoding:** Automatic probe and initialization for `VA-API` and `CUDA` zero-copy decoding via FFmpeg with graceful multi-threaded CPU software fallback.
- **⚡ Multi-Layer GPU Compositor (`GPUEngine`):** Offscreen Framebuffer Object (FBO) ping-pong renderer to process unbounded video layer stacks sequentially on GPU.
- **📐 4x4 Hardware Matrix Transformations:** Real-time affine matrix evaluation per layer:
  $$M = T(x, y) \cdot R(\theta) \cdot S(s_x, s_y) \cdot T(-a_x, -a_y)$$
- **🎨 Custom GLSL Effect & Blend Shaders:** Vectorized GLSL blend implementations (*Normal, Add, Multiply, Screen, Overlay, Soft Light, Color Dodge*) and 3-way Lift/Gamma/Gain + Exposure/Saturation color grading.
- **📈 Cubic Bézier Keyframe Interpolation:** Mathematical cubic Bézier curve evaluation ($P(t) = (1-t)^3 P_0 + 3(1-t)^2 t P_1 + 3(1-t) t^2 P_2 + t^3 P_3$) with Newton-Raphson solver for smooth easing curves.
- **🖥️ Qt 6 Scene Graph Viewport:** High-throughput `QQuickItem` viewport bridge rendering decoded frames and GPU composites directly within the Qt Quick scene graph.
- **📊 Headless Diagnostic CLIs:**
  - `cli_test_decoder`: Video decoding throughput (FPS), seek latency profiling, and RAM leak verification.
  - `cli_test_compositor`: Headless offscreen GPU multi-layer compositing benchmark for 1080p and 4K UHD workloads.

---

## 📁 Project Structure

```
.
├── CMakeLists.txt              # C++20 build system & dependency configuration
├── README.md                   # Project documentation
├── .gitignore                  # Git ignore rules
├── src/
│   ├── main.cpp                # GUI Application entry point & QML bridge
│   ├── core/
│   │   ├── DecoderEngine.hpp   # FFmpeg HW decoding wrapper & RAII types
│   │   ├── DecoderEngine.cpp   # VA-API/CUDA decoding & frame ring buffer
│   │   ├── GraphEngine.hpp     # DAG composition & cubic Bézier keyframes
│   │   ├── GraphEngine.cpp     # Topological sorting & software rasterizer
│   │   ├── GPUEngine.hpp       # GPU ping-pong FBOs & 4x4 matrix math
│   │   └── GPUEngine.cpp       # GLSL shader manager & multi-layer compositor
│   ├── shaders/
│   │   ├── compositor.vert     # Layer matrix transformation vertex shader
│   │   ├── blend_modes.frag    # Vectorized blend modes & Porter-Duff alpha
│   │   └── color_grade.frag    # CDL 3-way color grading & tone adjustments
│   └── ui/
│       ├── ViewportItem.hpp    # QQuickItem Scene Graph rendering bridge
│       ├── ViewportItem.cpp    # Viewport paint node & playback controller
│       └── main.qml            # Responsive dark-themed NLE studio UI
└── tests/
    ├── cli_test_decoder.cpp    # Headless decoder diagnostic & benchmark tool
    └── cli_test_compositor.cpp # Headless GPU multi-layer compositing benchmark
```

---

## 🛠️ Prerequisites

Ensure you have the required development packages installed:

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-declarative-dev libavcodec-dev libavformat-dev \
    libavutil-dev libswscale-dev libswresample-dev libva-dev libgl-dev
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake ninja pkgconf qt6-base qt6-declarative ffmpeg libva
```

### Fedora / RHEL
```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig \
    qt6-qtbase-devel qt6-qtdeclarative-devel ffmpeg-free-devel libva-devel
```

---

## 🔨 Building the Project

```bash
# Configure build directory with Ninja
cmake -B build -G Ninja

# Compile all targets (CLI tools & GUI editor)
ninja -C build
```

---

## 🚀 Running

### 1. Headless GPU Multi-Layer Compositing Benchmark
Run offscreen GPU benchmarks across 1080p and 4K UHD 5-layer compositing workloads:

```bash
./build/cli_test_compositor
```

### 2. Headless Decoder Diagnostic & Benchmark
Run hardware decoder performance benchmarks directly from your terminal:

```bash
# Run with automatic synthetic test stream generation:
./build/cli_test_decoder

# Or benchmark a specific video file:
./build/cli_test_decoder /path/to/video.mp4 --frames 500
```

### 3. Video Editor GUI Application
Launch the NLE workspace:

```bash
./build/video_editor
```

---

## 📜 License

MIT License. Free for open-source and commercial use.
