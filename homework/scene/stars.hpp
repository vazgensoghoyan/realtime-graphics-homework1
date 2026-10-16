#pragma once

#include <math/aliases.hpp>

#include <cstdint>
#include <vector>

struct StarInstance {
    float radius;
    math::vector3f center;
};

// generated on a sphere
std::vector<StarInstance> generateStars(uint32_t seed, uint32_t starsCount, uint32_t radius = 50);
