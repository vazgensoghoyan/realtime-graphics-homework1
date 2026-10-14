Русский | [English](README.en.md)

# Планетная система

Учебная 3D-сцена на C++20 и WebGPU: планеты, спутники, кольца, звёзды и анимированное Солнце. [Задание](homework01.pdf).

**Управление:** WASD — движение, мышь — обзор, Space — ускорение, Esc — выход.

## Сборка (Linux)

Нужны CMake 3.20+, SDL3 и wgpu-native 29.0.1.1. Из корня репозитория, указав пути к установленным библиотекам:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/SDL3 \
  -DWGPU_NATIVE_ROOT=/path/to/wgpu-native
cmake --build build --target src --parallel
./build/homework/src
```

Основа: [realtime-graphics-course-v2-practice](https://github.com/lisyarus/realtime-graphics-course-v2-practice).
