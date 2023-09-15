#pragma once

#include <Utils/Types.h>

namespace cassidy
{
  // Forward declarations:
  class Engine;

  class Camera
  {
  public:
    Camera();

    void init(Engine* engineRef);

    void update();
    inline glm::mat4 getLookatMatrix()      { return m_lookat; }
    inline glm::mat4 getPerspectiveMatrix() { return m_proj; }

    void moveForward(float speedScalar = 1.0f);
    void moveRight(float speedScalar = 1.0f);
    void moveWorldUp(float speedScalar = 1.0f);
    void moveUp(float speedScalar = 1.0f);

    void turnLeft();
    void turnRight();
    void lookUp();
    void lookDown();

  private:
    void findForward();
    void calculateLookat();
    void updateProj();

    Engine* m_engineRef;

    glm::mat4 m_lookat;
    glm::mat4 m_proj;

    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;

    glm::vec3 m_eulerAngles;  // (Pitch, Yaw, Roll), in degrees

    float m_fovDegrees;
    float m_rotateSensitivity;
    float m_moveSpeed;
    glm::vec2 m_clipPlaneValues;

    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
  };
}
