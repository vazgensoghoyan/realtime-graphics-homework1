#include "camera.hpp"

#include <math/affine.hpp>
#include <math/projection.hpp>

void Camera::handleEvent(SDL_Event const &event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        m_keydown.insert(event.key.key);
        break;
    case SDL_EVENT_KEY_UP:
        m_keydown.erase(event.key.key);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        m_yaw += event.motion.xrel * s_mouseSensitivity;
        m_pitch -= event.motion.yrel * s_mouseSensitivity;
        m_pitch = math::clamp(m_pitch, -s_pitchLimit, s_pitchLimit);
        break;
    }
}

void Camera::update(float dt) {
    float const step = (m_keydown.contains(SDLK_SPACE) ? s_fastSpeed : s_speed) * dt;

    if (m_keydown.contains(SDLK_W)) m_position.z() -= step;
    if (m_keydown.contains(SDLK_S)) m_position.z() += step;
    if (m_keydown.contains(SDLK_A)) m_position.x() -= step;
    if (m_keydown.contains(SDLK_D)) m_position.x() += step;
}

math::matrix4f Camera::viewMatrix() const {
    return math::rotation_yz(-m_pitch)
         * math::rotation_xz(-m_yaw)
         * math::translation(-m_position);
}

math::matrix4f Camera::projectionMatrix(float aspectRatio) const {
    return math::perspective(s_verticalFov, aspectRatio, s_nearPlane, s_farPlane);
}
