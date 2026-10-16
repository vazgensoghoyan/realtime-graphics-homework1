#include "colors.hpp"

#include <cmath>

math::vector4f bodyVertexColor(const math::vector3f &position, const math::vector4f &color, float seed) {
    const auto p = math::normalized(position);
    const float pattern = std::sin(p[0] * 6.f + seed)
                        * std::sin(p[1] * 6.f + seed * 0.37f)
                        * std::sin(p[2] * 6.f + seed * 0.73f);
    const float brightness = std::lerp(0.55f, 1.f, 0.5f + 0.5f * pattern);
    return {color[0] * brightness, color[1] * brightness, color[2] * brightness, color[3]};
}
