#pragma once

#include <math/aliases.hpp>

#include <cstdint>
#include <vector>

struct SphereVertex {
    math::vector3f position;
    math::vector3f normal;
};

struct SphereMesh {
    std::vector<SphereVertex> vertices;
    std::vector<std::uint32_t> indices;
};

SphereMesh generateSphere(std::uint32_t sectors, std::uint32_t stacks);
