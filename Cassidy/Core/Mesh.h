#pragma once

#include <Utils/Types.h>

namespace cassidy
{
  class Renderer;

  class Mesh
  {
  public:
    void release(VmaAllocator allocator);

    void allocateVertexBuffer(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline uint32_t getNumVertices() const    { return static_cast<uint32_t>(m_vertices.size()); }
    inline Vertex const* getVertices() const  { return m_vertices.data(); }

  private:
    std::vector<Vertex> m_vertices;
    AllocatedBuffer m_vertexBuffer;
  };
}