#pragma once

#include <Utils/Types.h>

// Forward declarations:
struct aiNode;
struct aiScene;
struct aiMesh;

namespace cassidy
{
  class Renderer;

  class Mesh
  {
  public:
    void release(VmaAllocator allocator);

    void loadMesh(const std::string& filepath);
    void setVertices(const std::vector<Vertex>& vertices);

    void allocateVertexBuffer(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline uint32_t getNumVertices() const      { return static_cast<uint32_t>(m_vertices.size()); }
    inline Vertex const* getVertices() const    { return m_vertices.data(); }
    inline AllocatedBuffer const* getBuffer() const   { return &m_vertexBuffer; }

  private:
    std::vector<Vertex> m_vertices;
    AllocatedBuffer m_vertexBuffer;

    void processSceneNode(aiNode* node, const aiScene* scene);
    void processMesh(const aiMesh* mesh);
  };
}