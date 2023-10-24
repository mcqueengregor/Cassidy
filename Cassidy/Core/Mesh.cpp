#include "Mesh.h"
#include <Utils/Initialisers.h>
#include <Core/Renderer.h>
#include <Vendor/Assimp-5.3.1/assimp/Importer.hpp>
#include <Vendor/Assimp-5.3.1/assimp/scene.h>
#include <Vendor/Assimp-5.3.1/assimp/postprocess.h>

#include <iostream>

void cassidy::Mesh::release(VmaAllocator allocator)
{
  vmaDestroyBuffer(allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
}

void cassidy::Mesh::loadMesh(const std::string& filepath)
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

void cassidy::Mesh::setVertices(const std::vector<Vertex>& vertices)
{
  m_vertices.assign(vertices.begin(), vertices.end());
}

void cassidy::Mesh::allocateVertexBuffer(VkCommandBuffer uploadCmd, VmaAllocator allocator, cassidy::Renderer* rendererRef)
{
  // Build CPU-side staging buffer:
  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(m_vertices.size() * sizeof(Vertex),
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
  memcpy(data, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(m_vertices.size() * sizeof(Vertex),
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
    copy.size = m_vertices.size() * sizeof(Vertex);
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
  });

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

  m_vertexBuffer = newBuffer;
}

void cassidy::Mesh::processSceneNode(aiNode* node, const aiScene* scene)
{
  // Iterate over all meshes, merge into common vertex buffer:
  for (uint32_t i = 0; i < node->mNumMeshes; ++i)
  {
    const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    processMesh(mesh);
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
  for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
  {
    {
      glm::vec3 position;

      position.x = mesh->mVertices[i].x;
      position.y = mesh->mVertices[i].y;
      position.z = mesh->mVertices[i].z;
      vertex.position = position;
    }

    if (mesh->HasTextureCoords(1))
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
}
