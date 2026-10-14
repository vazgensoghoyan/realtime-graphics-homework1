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

    math::vector3f position(float time) const;
    math::matrix4f modelMatrix(float time) const;
};

// sun is first; same seed reproduces same scene
std::vector<CelestialBody> generateScene(uint32_t seed);
