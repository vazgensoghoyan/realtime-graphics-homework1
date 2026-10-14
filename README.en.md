[Русский](README.md) | English

# Planetary System

A 3D computer graphics coursework project in C++20 and WebGPU: planets, moons, rings, stars, and an animated Sun. [Assignment (Russian)](homework01.pdf).

**Controls:** WASD — move, mouse — look, Space — speed up, Esc — exit.

## Build (Linux)

Requires CMake 3.20+, SDL3, and wgpu-native 29.0.1.1. Run from the repository root, setting the paths to your installed libraries:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/SDL3 \
  -DWGPU_NATIVE_ROOT=/path/to/wgpu-native
cmake --build build --target src --parallel
./build/homework/src
```

Based on [realtime-graphics-course-v2-practice](https://github.com/lisyarus/realtime-graphics-course-v2-practice).
