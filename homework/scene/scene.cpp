#include "scene.hpp"

#include <math/affine.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <stdexcept>

math::vector3f CelestialBody::position(float time, const std::vector<CelestialBody>& bodies) const {
    math::vector3f worldPosition{};
    const CelestialBody* body = this;
    const CelestialBody* sun = &bodies.at(0);

    for (size_t depth = 0; body != sun; ++depth) {
        if (depth >= bodies.size())
            throw std::invalid_argument("Cycle in celestial body parents");

        const float angle = body->orbitPhase + body->speed * time;

        worldPosition += math::vector3f{
            body->orbitRadius * std::cos(angle),
            0.f,
            body->orbitRadius * std::sin(angle)
        };

        body = &bodies.at(body->parentIndex);
    }

    return worldPosition;
}

math::matrix4f CelestialBody::modelMatrix(float time, const std::vector<CelestialBody>& bodies) const {
    const float spinAngle = spinSpeed * time;
    return math::translation(position(time, bodies))
         * math::rotation_xz(spinAngle)
         * math::scale(math::vector3f{radius, radius, radius});
}

math::vector4f CelestialBody::vertexColor(const math::vector3f &localPosition) const {
    const auto p = math::normalized(localPosition);
    const float pattern = std::sin(p[0] * 6.f + materialSeed)
                        * std::sin(p[1] * 6.f + materialSeed * 0.37f)
                        * std::sin(p[2] * 6.f + materialSeed * 0.73f);
    const float brightness = std::lerp(0.55f, 1.f, 0.5f + 0.5f * pattern);
    return {color[0] * brightness, color[1] * brightness, color[2] * brightness, color[3]};
}

std::vector<CelestialBody> generateScene(uint32_t seed) {
    constexpr float sceneScale = 0.5f;
    constexpr size_t planetCount = 10;
    constexpr float orbitSpacing = 6.f;
    constexpr float sunRadius = 9.f;
    constexpr float orbitOffset = sunRadius;
    constexpr float tau = 2.f * std::numbers::pi_v<float>;

    std::mt19937 random(seed);
    auto sample = [&random](float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(random);
    };

    std::array<math::vector4f, planetCount> colors{{
        {0.4f, 0.65f, 1.f, 1.f},
        {1.f, 0.4f, 0.3f, 1.f},
        {0.35f, 1.f, 0.45f, 1.f},
        {0.85f, 0.4f, 1.f, 1.f},
        {0.3f, 1.f, 1.f, 1.f},
        {1.f, 0.65f, 0.3f, 1.f},
        {1.f, 0.5f, 0.8f, 1.f},
        {0.7f, 1.f, 0.3f, 1.f},
        {0.55f, 0.45f, 1.f, 1.f},
        {1.f, 0.85f, 0.65f, 1.f},
    }};
    std::shuffle(colors.begin(), colors.end(), random);

    std::vector<CelestialBody> bodies;
    bodies.reserve(planetCount + 4);

    // adding SUN

    bodies.push_back({
        .radius = sunRadius * sceneScale,
        .color = {1.f, 0.8f, 0.15f, 1.f},
    });

    // generating planets

    for (size_t i = 1; i <= planetCount; ++i) {
        float const orbitRadius = orbitOffset + i * orbitSpacing + sample(-0.25f, 0.25f);
        float const radius = 0.30f + 0.15f * i + sample(-0.15f, 0.15f);
        float const phase = sample(0.f, tau);
        float const speed = 0.8f * std::pow(0.78f, static_cast<float>(i - 1)) * sample(0.95f, 1.05f);

        bodies.push_back({
            .radius = radius * sceneScale,
            .color = colors[i - 1],
            .speed = speed,
            .orbitRadius = orbitRadius * sceneScale,
            .orbitPhase = phase,
        });
    }

    // moons

    bodies.push_back({
        .radius = bodies[4].radius * 0.3f,
        .color = {0.85f, 0.85f, 0.9f, 1.f},
        .speed = 1.5f,
        .orbitRadius = bodies[4].radius * 4.f,
        .orbitPhase = sample(0.f, tau),
        .parentIndex = 4,
    });

    bodies.push_back({
        .radius = bodies[6].radius * 0.25f,
        .color = {0.8f, 0.85f, 0.95f, 1.f},
        .speed = 1.5f,
        .orbitRadius = bodies[6].radius * 2.f,
        .orbitPhase = sample(0.f, tau),
        .parentIndex = 6,
    });

    bodies.push_back({
        .radius = bodies[6].radius * 0.2f,
        .color = {0.95f, 0.8f, 0.65f, 1.f},
        .speed = 0.9f,
        .orbitRadius = bodies[6].radius * 3.5f,
        .orbitPhase = sample(0.f, tau),
        .parentIndex = 6,
    });

    for (auto &body : bodies) {
        body.materialSeed = sample(0.f, 1000.f);
    }

    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].spinSpeed = i == 0 ? 0.2f : sample(0.35f, 1.2f);
    }

    return bodies;
}
