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
    void release(VmaAllocator allocator) const;

    void processMesh(const aiMesh* mesh);
    void setVertices(const std::vector<Vertex>& vertices);

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline uint32_t getNumVertices() const    { return static_cast<uint32_t>(m_vertices.size()); }
    inline uint32_t getNumIndices() const     { return static_cast<uint32_t>(m_indices.size()); }
    inline Vertex const* getVertices() const  { return m_vertices.data(); }
    inline uint32_t const* getIndices() const { return m_indices.data(); }
    inline AllocatedBuffer const* getVertexBuffer() const { return &m_vertexBuffer; }
    inline AllocatedBuffer const* getIndexBuffer() const  { return &m_indexBuffer; }

    inline void setVertexBuffer(AllocatedBuffer newBuffer) { m_vertexBuffer = newBuffer; }
    inline void setIndexBuffer(AllocatedBuffer newBuffer) { m_indexBuffer = newBuffer; }

  private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    AllocatedBuffer m_vertexBuffer;
    AllocatedBuffer m_indexBuffer;
  };

  class Model
  {
  public:
    void draw(VkCommandBuffer cmd);

    void release(VmaAllocator allocator);

    void loadModel(const std::string& filepath);
    void setVertices(const std::vector<Vertex>& vertices);

    void allocateVertexBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);
    void allocateIndexBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);

  private:
    void processSceneNode(aiNode* node, const aiScene* scene);

    std::vector<Mesh> m_meshes;
  };
}