#include "Mesh.h"
#include <Utils/Initialisers.h>
#include <Core/Renderer.h>
#include <Vendor/Assimp-5.3.1/assimp/Importer.hpp>
#include <Vendor/Assimp-5.3.1/assimp/scene.h>
#include <Vendor/Assimp-5.3.1/assimp/postprocess.h>

#include <iostream>

void cassidy::Model::draw(VkCommandBuffer cmd)
{
  for (const auto& mesh : m_meshes)
  {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.getVertexBuffer()->buffer, &offset);
    vkCmdBindIndexBuffer(cmd, mesh.getIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.getNumIndices()), 1, 0, 0, 0);
  }
}

void cassidy::Model::release(VmaAllocator allocator)
{
  for (const auto& mesh : m_meshes)
  {
    mesh.release(allocator);
  }
}

void cassidy::Model::loadModel(const std::string& filepath)
{
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(filepath,
    aiProcess_Triangulate |
    aiProcess_CalcTangentSpace |
    aiProcess_GenSmoothNormals);

  if (!scene)
  {
    std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
    return;
  }

  processSceneNode(scene->mRootNode, scene);

  std::cout << "Successfully loaded mesh " << filepath << "!" << std::endl;
}

// Used for single-mesh models which have their vertices directly set by an array.
void cassidy::Model::setVertices(const std::vector<Vertex>& vertices)
{
  if (m_meshes.empty())
  {
    m_meshes.emplace_back(Mesh());
    m_meshes[0].setVertices(vertices);
  }
}

void cassidy::Model::allocateVertexBuffers(VkCommandBuffer uploadCmd, VmaAllocator allocator, cassidy::Renderer* rendererRef)
{
  for (auto& mesh : m_meshes)
  {
    // Build CPU-side staging buffer:
    VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(mesh.getNumVertices() * sizeof(Vertex),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    AllocatedBuffer stagingBuffer;

    vmaCreateBuffer(allocator, &stagingBufferInfo, &bufferAllocInfo,
      &stagingBuffer.buffer,
      &stagingBuffer.allocation,
      nullptr);

    // Write vertex data to newly-allocated buffer:
    void* data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, mesh.getVertices(), mesh.getNumVertices() * sizeof(Vertex));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(mesh.getNumVertices() * sizeof(Vertex),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    AllocatedBuffer newBuffer;

    vmaCreateBuffer(allocator, &vertexBufferInfo, &bufferAllocInfo,
      &newBuffer.buffer,
      &newBuffer.allocation,
      nullptr);

    // Execute copy command for CPU-side staging buffer -> GPU-side vertex buffer:
    rendererRef->immediateSubmit([=](VkCommandBuffer cmd) {
      VkBufferCopy copy = {};
      copy.dstOffset = 0;
      copy.srcOffset = 0;
      copy.size = mesh.getNumVertices() * sizeof(Vertex);
      vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
    });

    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    mesh.setVertexBuffer(newBuffer);
  }
}

void cassidy::Model::allocateIndexBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef)
{
  for (auto& mesh : m_meshes)
  {
    // Build CPU-side staging buffer:
    VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(mesh.getNumIndices() * sizeof(uint32_t),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    AllocatedBuffer stagingBuffer;

    vmaCreateBuffer(allocator, &stagingBufferInfo, &bufferAllocInfo,
      &stagingBuffer.buffer,
      &stagingBuffer.allocation,
      nullptr);

    // Write index data to newly-allocated buffer:
    void* data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, mesh.getIndices(), mesh.getNumIndices() * sizeof(uint32_t));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    VkBufferCreateInfo indexBufferInfo = cassidy::init::bufferCreateInfo(mesh.getNumIndices() * sizeof(uint32_t),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    AllocatedBuffer newBuffer;

    vmaCreateBuffer(allocator, &indexBufferInfo, &bufferAllocInfo,
      &newBuffer.buffer,
      &newBuffer.allocation,
      nullptr);

    // Execute copy command for CPU-side staging buffer -> GPU-side vertex buffer:
    rendererRef->immediateSubmit([=](VkCommandBuffer cmd) {
      VkBufferCopy copy = {};
      copy.dstOffset = 0;
      copy.srcOffset = 0;
      copy.size = mesh.getNumIndices() * sizeof(uint32_t);
      vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
    });

    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    mesh.setIndexBuffer(newBuffer);
  }
}

void cassidy::Model::processSceneNode(aiNode* node, const aiScene* scene)
{
  m_meshes.reserve(node->mNumMeshes);

  // Iterate over all meshes, merge into common vertex buffer:
  for (uint32_t i = 0; i < node->mNumMeshes; ++i)
  {
    const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    m_meshes.emplace_back(Mesh());
    m_meshes[m_meshes.size() - 1].processMesh(mesh);
  }

  // Recursively iterate over child nodes and their meshes:
  for (uint32_t i = 0; i < node->mNumChildren; ++i)
  {
    processSceneNode(node->mChildren[i], scene);
  }
}

void cassidy::Mesh::processMesh(const aiMesh* mesh)
{
  Vertex vertex;

  // Store the vertex data in the current mesh, add to merged vertex buffer:
  m_vertices.reserve(mesh->mNumVertices);

  for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
  {
    {
      glm::vec3 position;

      position.x = mesh->mVertices[i].x;
      position.y = mesh->mVertices[i].y;
      position.z = mesh->mVertices[i].z;
      vertex.position = position;
    }

    if (mesh->HasTextureCoords(0))
    {
      glm::vec2 uv;

      uv.x = mesh->mTextureCoords[0][i].x;
      uv.y = mesh->mTextureCoords[0][i].y;
      vertex.uv = uv;
    }

    if (mesh->HasNormals())
    {
      glm::vec3 normal;

      normal.x = mesh->mNormals[i].x;
      normal.y = mesh->mNormals[i].y;
      normal.z = mesh->mNormals[i].z;
      vertex.normal = normal;
    }

    m_vertices.emplace_back(vertex);
  }

  // Retrieve index data from mesh faces:
  m_indices.reserve(mesh->mNumFaces);

  for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
  {
    const aiFace face = mesh->mFaces[i];
    for (uint32_t j = 0; j < face.mNumIndices; ++j)
    {
      m_indices.push_back(face.mIndices[j]);
    }
  }
}

void cassidy::Mesh::release(VmaAllocator allocator) const
{
  vmaDestroyBuffer(allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
  vmaDestroyBuffer(allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
}

void cassidy::Mesh::setVertices(const std::vector<Vertex>& vertices)
{
  m_vertices.assign(vertices.begin(), vertices.end());
}
