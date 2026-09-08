# Headless-Editor ⚡

A high-performance, lightweight Linux Video Editor core, GPU Compositing Engine, and Native PipeWire Audio Playback Subsystem implemented in **C++20**, **OpenGL / GLSL**, **PipeWire API**, **Qt 6 (QML)**, and **FFmpeg**. Built for hardware-accelerated video decoding (VA-API / CUDA), multi-layer GPU ping-pong compositing, 4x4 matrix transformations, cubic Bézier keyframe animation, low-latency 48kHz audio rendering, and master clock A/V synchronization.

---

## ✨ Features

- **🚀 Hardware-Accelerated Decoding:** Automatic probe and initialization for `VA-API` and `CUDA` zero-copy decoding via FFmpeg with graceful multi-threaded CPU software fallback.
- **⚡ Multi-Layer GPU Compositor (`GPUEngine`):** Offscreen Framebuffer Object (FBO) ping-pong renderer to process unbounded video layer stacks sequentially on GPU.
- **🔊 Native PipeWire Low-Latency Audio (`AudioEngine`):** Real-time stereo float 48kHz (`SPA_AUDIO_FORMAT_F32`) output stream powered by `pw_thread_loop`, `libswresample`, and a lock-free SPSC ring buffer (~10.67 ms buffer latency).
- **⏱️ Master Clock & Frame-Accurate A/V Sync (`TimelineController`):** Precision clock synchronization driven by hardware audio PTS with fallback to monotonic system clock. Exact SMPTE timecode generation (`HH:MM:SS:FF`) and variable rate shuttle scrubbing (1x, 2x, 4x, -1x).
- **🎛️ Interactive Multi-Track QML Timeline:** Track lanes (`V2`, `V1`, `A1`, `A2`, `FX1`), draggable and trimmable clips, waveform visualization, and inline cubic Bézier curve overlays with draggable `KeyframeHandle` nodes.
- **📐 4x4 Hardware Matrix Transformations:** Real-time affine matrix evaluation per layer:
  $$M = T(x, y) \cdot R(\theta) \cdot S(s_x, s_y) \cdot T(-a_x, -a_y)$$
- **🎨 Custom GLSL Shaders:** Vectorized blend modes (*Normal, Add, Multiply, Screen, Overlay, Soft Light, Color Dodge*) and 3-way Lift/Gamma/Gain color grading.
- **🖥️ Qt 6 Scene Graph Viewport:** High-throughput `QQuickItem` viewport bridge rendering decoded frames and GPU composites directly within the Qt Quick scene graph.
- **📊 Headless Diagnostic CLIs:**
  - `cli_test_decoder`: Video decoding throughput (FPS), seek latency profiling, and RAM leak verification.
  - `cli_test_compositor`: Headless offscreen GPU multi-layer compositing benchmark for 1080p and 4K UHD workloads.
  - `cli_test_audio_sync`: Native PipeWire buffer latency, libswresample verification, and master clock A/V drift benchmark.

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
│   │   ├── GPUEngine.cpp       # GLSL shader manager & multi-layer compositor
│   │   ├── AudioEngine.hpp     # Native PipeWire stream & lock-free ring buffer
│   │   ├── AudioEngine.cpp     # libswresample 48kHz audio resampler & feeder
│   │   ├── TimelineController.hpp # Master clock sync & SMPTE timecode
│   │   └── TimelineController.cpp # Transport state & A/V drift tracking
│   ├── shaders/
│   │   ├── compositor.vert     # Layer matrix transformation vertex shader
│   │   ├── blend_modes.frag    # Vectorized blend modes & Porter-Duff alpha
│   │   └── color_grade.frag    # CDL 3-way color grading & tone adjustments
│   └── ui/
│       ├── ViewportItem.hpp    # QQuickItem Scene Graph rendering bridge
│       ├── ViewportItem.cpp    # Viewport paint node & playback controller
│       ├── TimelineView.qml    # Master multi-track timeline container
│       ├── TimelineTrack.qml   # Individual track lane (V1/V2/A1/A2/FX)
│       ├── TimelineClip.qml    # Draggable/resizable clip item & waveform
│       ├── KeyframeHandle.qml  # Interactive keyframe node for Bézier curves
│       └── main.qml            # Responsive dark-themed NLE studio UI
└── tests/
    ├── cli_test_decoder.cpp    # Headless decoder diagnostic & benchmark tool
    ├── cli_test_compositor.cpp # Headless GPU multi-layer compositing benchmark
    └── cli_test_audio_sync.cpp # Headless PipeWire audio & A/V sync benchmark
```

---

## 🛠️ Prerequisites

Ensure you have the required development packages installed:

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-declarative-dev libavcodec-dev libavformat-dev \
    libavutil-dev libswscale-dev libswresample-dev libva-dev libgl-dev \
    libpipewire-0.3-dev libspa-0.2-dev
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake ninja pkgconf qt6-base qt6-declarative ffmpeg libva pipewire
```

### Fedora / RHEL
```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig \
    qt6-qtbase-devel qt6-qtdeclarative-devel ffmpeg-free-devel libva-devel pipewire-devel
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

### 1. PipeWire Audio & Master Clock A/V Sync Benchmark
Test PipeWire low-latency stream creation, resampler throughput, and sub-frame clock alignment:

```bash
# Run with synthetic 48kHz sine tone:
./build/cli_test_audio_sync

# Or benchmark with an audio/video media file:
./build/cli_test_audio_sync /path/to/media.mp4 --duration 5.0
```

### 2. Headless GPU Multi-Layer Compositing Benchmark
Run offscreen GPU benchmarks across 1080p and 4K UHD 5-layer compositing workloads:

```bash
./build/cli_test_compositor
```

### 3. Headless Decoder Diagnostic & Benchmark
Run hardware decoder performance benchmarks directly from your terminal:

```bash
# Run with automatic synthetic test stream generation:
./build/cli_test_decoder

# Or benchmark a specific video file:
./build/cli_test_decoder /path/to/video.mp4 --frames 500
```

### 4. Video Editor GUI Application
Launch the NLE workspace:

```bash
./build/video_editor
```

---

## 📜 License

MIT License. Free for open-source and commercial use.
