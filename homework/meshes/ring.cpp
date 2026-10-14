#include "ring.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

ObjMesh generateRing(float innerRadius, float outerRadius, uint32_t segments) {
    uint64_t const vertexCount = 2 * segments;
    uint64_t const indexCount = 6 * segments;

    ObjMesh mesh;
    mesh.vertices.reserve(vertexCount);
    mesh.indices.reserve(indexCount);

    for (uint32_t i = 0; i < segments; ++i) {
        double const angle = 2.0 * std::numbers::pi * i / segments;
        float const x = static_cast<float>(std::cos(angle));
        float const z = static_cast<float>(std::sin(angle));

        constexpr math::vector3f yNormal = {0.f, 1.f, 0.f};
        mesh.vertices.push_back({{innerRadius * x, 0.f, innerRadius * z}, yNormal, {}});
        mesh.vertices.push_back({{outerRadius * x, 0.f, outerRadius * z}, yNormal, {}});
    }

    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t const inner = 2 * i;
        uint32_t const outer = inner + 1;
        uint32_t const nextInner = 2 * ((i + 1) % segments);
        uint32_t const nextOuter = nextInner + 1;

        mesh.indices.insert(mesh.indices.end(), {
            inner, nextInner, outer,
            outer, nextInner, nextOuter,
        });
    }

    return mesh;
}
