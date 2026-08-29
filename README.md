# Headless-Editor ⚡

A high-performance, lightweight Linux Video Editor core implemented in **C++20**, **Qt 6 (QML)**, and **FFmpeg**. Built for hardware-accelerated video decoding (VA-API / CUDA), node-based DAG composition, cubic Bézier keyframe animation, and real-time preview viewport rendering.

---

## ✨ Features

- **🚀 Hardware-Accelerated Decoding:** Automatic probe and initialization for `VA-API` and `CUDA` zero-copy decoding via FFmpeg with graceful multi-threaded CPU software fallback.
- **🛡️ RAII Memory Architecture:** Clean C++20 smart pointer wrappers for all FFmpeg contexts (`AVFormatContext`, `AVCodecContext`, `AVPacket`, `AVFrame`, `AVBufferRef`, `SwsContext`).
- **🔀 DAG Compositing Engine:** Directed Acyclic Graph supporting media clips, 2D transforms, color grading filters, blend modes (*Normal, Add, Multiply, Screen, Overlay*), and cycle detection.
- **📈 Cubic Bézier Keyframe Interpolation:** Mathematical cubic Bézier curve evaluation ($P(t) = (1-t)^3 P_0 + 3(1-t)^2 t P_1 + 3(1-t) t^2 P_2 + t^3 P_3$) with Newton-Raphson solver for smooth easing curves.
- **🖥️ Qt 6 Scene Graph Viewport:** High-throughput `QQuickItem` viewport bridge rendering decoded frames and DAG composites directly within the Qt Quick scene graph.
- **📊 Headless Diagnostic CLI:** Standalone terminal benchmark tool (`cli_test_decoder`) for throughput (FPS), seek latency profiling, and RAM leak verification.

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
│   │   └── GraphEngine.cpp     # Topological sorting & software rasterizer
│   └── ui/
│       ├── ViewportItem.hpp    # QQuickItem Scene Graph rendering bridge
│       ├── ViewportItem.cpp    # Viewport paint node & playback controller
│       └── main.qml            # Responsive dark-themed NLE studio UI
└── tests/
    └── cli_test_decoder.cpp    # Headless performance diagnostic & benchmark tool
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

# Compile targets (CLI tool & GUI editor)
ninja -C build
```

---

## 🚀 Running

### 1. Headless CLI Diagnostic & Benchmark
Run performance benchmarks and hardware accelerator checks directly from your terminal:

```bash
# Run with automatic synthetic test stream generation:
./build/cli_test_decoder

# Or benchmark a specific video file:
./build/cli_test_decoder /path/to/video.mp4 --frames 500
```

### 2. Video Editor GUI Application
Launch the NLE workspace:

```bash
./build/video_editor
```

---

## 📜 License

MIT License. Free for open-source and commercial use.
