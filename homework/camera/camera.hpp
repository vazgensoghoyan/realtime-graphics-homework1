#pragma once

#include <SDL3/SDL.h>
#include <math/aliases.hpp>

#include <numbers>
#include <unordered_set>

class Camera {
public:
    void handleEvent(SDL_Event const &event);
    void update(float dt);

    math::matrix4f viewMatrix() const;
    math::matrix4f projectionMatrix(float aspectRatio) const;

private:
    static constexpr float s_speed = 2.f;
    static constexpr float s_fastSpeed = 5.f;
    static constexpr float s_mouseSensitivity = 0.003f;
    static constexpr float s_pitchLimit = std::numbers::pi_v<float> / 2.f - 0.01f;
    static constexpr float s_verticalFov = std::numbers::pi_v<float> / 3.f;
    static constexpr float s_nearPlane = 0.1f;
    static constexpr float s_farPlane = 100.f;

    math::vector3f m_position{0.f, 0.f, 3.f};
    float m_yaw = 0.f;
    float m_pitch = 0.f;

    std::unordered_set<SDL_Keycode> m_keydown;
};
