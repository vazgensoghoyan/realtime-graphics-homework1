#pragma once

#include <math/aliases.hpp>

#include <cstdint>
#include <vector>

struct CelestialBody {
    float radius;
    float speed;
    math::vector4f color;

    float orbitRadius;
    float orbitPhase;

    uint32_t parentIndex;

    math::vector3f position(float time, const std::vector<CelestialBody>& bodies) const;
    math::matrix4f modelMatrix(float time, const std::vector<CelestialBody>& bodies) const;
};

// sun is first; same seed reproduces same scene
std::vector<CelestialBody> generateScene(uint32_t seed);
