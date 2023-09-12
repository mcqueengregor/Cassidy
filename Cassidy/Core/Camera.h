#pragma once

#include <Utils/Types.h>

class Camera
{
public:


private:
  glm::vec3 m_position;
  glm::vec3 m_forward;
  glm::vec3 m_up;
  glm::vec3 m_right;

  float m_fovDegrees;
  float m_rotateSensitivity;

  float m_moveSpeed;
  float m_fastMoveSpeed;

  const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

