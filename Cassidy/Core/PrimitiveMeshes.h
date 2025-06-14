#pragma once
#include <Utils/Types.h>

const std::vector<Vertex> TRIANGLE_VERTEX =
{
  {{ 0.0f,  0.5f, 0.0f},  {0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
  {{-0.5f, -0.5f, 0.0f},  {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
  {{ 0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
};

const std::vector<uint32_t> TRIANGLE_INDEX =
{
  0, 1, 2,
};
