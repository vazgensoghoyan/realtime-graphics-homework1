#include "stars.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

std::vector<StarInstance> generateStars(uint32_t seed, uint32_t radius) {
    constexpr size_t starsCount = 3000;

    std::mt19937 random(seed);

    auto heightDistribution = [&random]() {
        return std::uniform_real_distribution<float>(-1, 1)(random);
    };

    auto phiDistribution = [&random]() {
        constexpr float tau = 2 * std::numbers::pi_v<float>;
        return std::uniform_real_distribution<float>(0, tau)(random);
    };

    auto radiusDistribution = [&random]() {
        return std::uniform_real_distribution<float>(0.04f, 0.10f)(random);
    };

    std::vector<StarInstance> stars;
    stars.reserve(starsCount);

    for (size_t i = 0; i < starsCount; ++i) {
        const float h = heightDistribution();
        const float r = std::sqrt( std::max(0.f, 1 - h * h) );
        const float phi = phiDistribution();

        math::vector3f center = {
            radius * r * std::cos(phi),
            radius * h,
            radius * r * std::sin(phi)
        };

        stars.push_back(StarInstance{
            .radius = radiusDistribution(),
            .center = center,
        });
    }

    return stars;
}
