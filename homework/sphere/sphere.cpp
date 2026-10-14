#include "sphere.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

ObjMesh generateSphere(uint32_t sectors, uint32_t stacks) {
    if (sectors < 3 || stacks < 2) {
        throw std::invalid_argument("Sphere requires sectors >= 3 and stacks >= 2");
    }

    uint64_t const vertexCount = 2 + uint64_t{sectors} * (stacks - 1);
    if (vertexCount > std::numeric_limits<uint32_t>::max()) {
        throw std::length_error("Sphere has too many vertices for 32-bit indices");
    }
    uint64_t const indexCount = 6 * uint64_t{sectors} * (stacks - 1);

    ObjMesh mesh;
    if (vertexCount > mesh.vertices.max_size() || indexCount > mesh.indices.max_size()) {
        throw std::length_error("Sphere exceeds mesh storage limits");
    }
    mesh.vertices.reserve(vertexCount);
    mesh.indices.reserve(indexCount);

    mesh.vertices.push_back({{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {}});

    for (uint32_t stack = 1; stack < stacks; ++stack) {
        double const theta = std::numbers::pi * stack / stacks;
        double const ringRadius = std::sin(theta);
        float const y = static_cast<float>(std::cos(theta));

        for (uint32_t sector = 0; sector < sectors; ++sector) {
            double const phi = 2.0 * std::numbers::pi * sector / sectors;
            math::vector3f const position{
                static_cast<float>(ringRadius * std::cos(phi)),
                y,
                static_cast<float>(ringRadius * std::sin(phi)),
            };
            mesh.vertices.push_back({position, position, {}});
        }
    }

    auto const southPole = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0.f, -1.f, 0.f}, {0.f, -1.f, 0.f}, {}});

    for (uint32_t sector = 0; sector < sectors; ++sector) {
        uint32_t const next = (sector + 1) % sectors;
        mesh.indices.insert(mesh.indices.end(), {0, 1 + next, 1 + sector});
    }

    for (uint32_t stack = 0; stack < stacks - 2; ++stack) {
        uint32_t const upper = 1 + stack * sectors;
        uint32_t const lower = upper + sectors;

        for (uint32_t sector = 0; sector < sectors; ++sector) {
            uint32_t const next = (sector + 1) % sectors;
            mesh.indices.insert(mesh.indices.end(), {
                upper + sector, upper + next, lower + sector,
                upper + next, lower + next, lower + sector,
            });
        }
    }

    uint32_t const lastRing = southPole - sectors;
    for (uint32_t sector = 0; sector < sectors; ++sector) {
        uint32_t const next = (sector + 1) % sectors;
        mesh.indices.insert(mesh.indices.end(), {
            southPole, lastRing + sector, lastRing + next,
        });
    }

    return mesh;
}
