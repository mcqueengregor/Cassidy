#pragma once

#include <Utils/Types.h>
#include <Core/Material.h>
#include <unordered_map>

// Forward declarations:
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;
enum aiPostProcessSteps;

namespace cassidy
{
  class Renderer;
  class Texture;
  class Material;
  class Pipeline;

  class Mesh
  {
  public:
    void release(VkDevice device, VmaAllocator allocator) const;

    void processMesh(const aiMesh* mesh);
    cassidy::MaterialInfo buildMaterialInfo(const aiScene* scene, uint32_t matIndex, const std::string& texturesDirectory, cassidy::Renderer* rendererRef);

    inline void setMaterial(cassidy::Material* material) { m_material = material; }
    inline void setVertices(const std::vector<Vertex>& vertices) { m_vertices.assign(vertices.begin(), vertices.end()); }
    inline void setIndices(const std::vector<uint32_t>& indices) { m_indices.assign(indices.begin(), indices.end()); }

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline uint32_t                  getNumVertices()  const { return static_cast<uint32_t>(m_vertices.size()); }
    inline uint32_t                  getNumIndices()   const { return static_cast<uint32_t>(m_indices.size()); }
    inline Vertex             const* getVertices()     const { return m_vertices.data(); }
    inline uint32_t           const* getIndices()      const { return m_indices.data(); }
    inline AllocatedBuffer    const* getVertexBuffer() const { return &m_vertexBuffer; }
    inline AllocatedBuffer    const* getIndexBuffer()  const { return &m_indexBuffer; }
    inline cassidy::Material*        getMaterial()     const { return m_material; }

    inline void setVertexBuffer(AllocatedBuffer newBuffer) { m_vertexBuffer = newBuffer; }
    inline void setIndexBuffer(AllocatedBuffer newBuffer) { m_indexBuffer = newBuffer; }

  private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    AllocatedBuffer m_vertexBuffer;
    AllocatedBuffer m_indexBuffer;

    cassidy::Material* m_material;
  };

  class Model
  {
  public:
    void draw(VkCommandBuffer cmd, const Pipeline* pipeline);

    void release(VkDevice device, VmaAllocator allocator);

    void loadModel(const std::string& filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, aiPostProcessSteps additionalSteps = (aiPostProcessSteps)0);
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<uint32_t>& indices);

    void allocateVertexBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);
    void allocateIndexBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);

    inline LoadResult getLoadResult() { return m_loadResult; }

  private:
    typedef std::unordered_map<uint32_t, cassidy::Material*> BuiltMaterials;

    void processSceneNode(aiNode* node, const aiScene* scene, BuiltMaterials& builtMaterials, const std::string& directory, cassidy::Renderer* rendererRef);

    std::vector<Mesh> m_meshes;
    LoadResult m_loadResult;
  };
};