#include "Camera.h"
#include "Core/Engine.h"
#include "Utils/GlobalTimer.h"

cassidy::Camera::Camera() :
  m_position(glm::vec3(0.0f, 0.0f, 3.0f)),
  m_eulerAngles(glm::vec3(0.0f, -90.0f, 0.0f)),
  m_fovDegrees(70.0f),
  m_clipPlaneValues(glm::vec2(0.1f, 300.0f)),
  m_rotateSensitivity(100.0f),
  m_moveSpeed(1.0f)
{
}

void cassidy::Camera::init(Engine* engineRef)
{
  m_engineRef = engineRef;
  updateProj();
}

void cassidy::Camera::update()
{
  findForward();
  calculateLookat();
}

void cassidy::Camera::moveForward(float speedScalar)
{
  m_position += m_forward * m_moveSpeed * speedScalar * GlobalTimer::deltaTime();
}

void cassidy::Camera::moveRight(float speedScalar)
{
  m_position += m_right * m_moveSpeed * speedScalar * GlobalTimer::deltaTime();
}

void cassidy::Camera::moveWorldUp(float speedScalar)
{
  m_position += WORLD_UP * m_moveSpeed * speedScalar * GlobalTimer::deltaTime();
}

void cassidy::Camera::moveUp(float speedScalar)
{
  m_position += m_up * m_moveSpeed * speedScalar * GlobalTimer::deltaTime();
}

void cassidy::Camera::increaseYaw(float speedScalar)
{
  m_eulerAngles.y += m_rotateSensitivity * speedScalar * GlobalTimer::deltaTime();
}

void cassidy::Camera::increasePitch(float speedScalar)
{
  m_eulerAngles.x += m_rotateSensitivity * speedScalar * GlobalTimer::deltaTime();
  m_eulerAngles.x = glm::clamp(m_eulerAngles.x, -89.0f, 89.0f);
}

void cassidy::Camera::findForward()
{
  // Calculate forward vector by interpreting pitch, yaw and roll
  // values as polar coordinates and converting to cartesian:
  m_forward = glm::vec3(
    cos(glm::radians(m_eulerAngles.y)) * cos(glm::radians(m_eulerAngles.x)),
    sin(glm::radians(m_eulerAngles.x)),
    sin(glm::radians(m_eulerAngles.y)) * cos(glm::radians(m_eulerAngles.x))
  );

  m_forward = glm::normalize(m_forward);
  m_right = glm::normalize(glm::cross(m_forward, WORLD_UP));
  m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void cassidy::Camera::calculateLookat()
{
  m_lookat = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void cassidy::Camera::updateProj()
{
  m_proj = glm::perspective(glm::radians(m_fovDegrees),
    static_cast<float>(m_engineRef->getWindowDim().x) / static_cast<float>(m_engineRef->getWindowDim().y),
    m_clipPlaneValues.x, m_clipPlaneValues.y);
  m_proj[1][1] *= -1.0f;  // Invert clip space transformation's y-coord to match Vulkan's expectations.
}
