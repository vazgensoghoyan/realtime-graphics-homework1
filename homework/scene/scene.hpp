#pragma once

#include <math/aliases.hpp>

#include <cstdint>
#include <vector>

struct CelestialBody {
    float radius;
    math::vector4f color;

    float speed = 0.f;

    float orbitRadius = 0.f;
    float orbitPhase = 0.f;

    uint32_t parentIndex = 0;
    float materialSeed = 0.f;

    math::vector3f position(float time, const std::vector<CelestialBody>& bodies) const;
    math::matrix4f modelMatrix(float time, const std::vector<CelestialBody>& bodies) const;
};

// sun is first; same seed reproduces same scene
std::vector<CelestialBody> generateScene(uint32_t seed);
