#include "Mesh.h"
#include <Core/Renderer.h>
#include <Core/Pipeline.h>
#include <Core/ResourceManager.h>
#include <Utils/Initialisers.h>

#include <Vendor/assimp/include/assimp/Importer.hpp>
#include <Vendor/assimp/include/assimp/scene.h>
#include <Vendor/assimp/include/assimp/postprocess.h>

#include <iostream>

void cassidy::Model::draw(VkCommandBuffer cmd, const Pipeline* pipeline)
{
  constexpr cassidy::MaterialLibrary& matLibrary = cassidy::globals::g_resourceManager.materialLibrary;
  cassidy::Material* lastMaterial = nullptr;

  for (const auto& mesh : m_meshes)
  {
    cassidy::Material* meshMaterial = mesh.getMaterial();
    if (!meshMaterial) meshMaterial = matLibrary.getErrorMaterial();

    if (mesh.getMaterial() != lastMaterial)
    {
      VkDescriptorSet&& textureSet = mesh.getMaterial()->getTextureDescSet();

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(),
        2, 1, &textureSet, 0, nullptr);
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.getVertexBuffer()->buffer, &offset);
    vkCmdBindIndexBuffer(cmd, mesh.getIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.getNumIndices()), 1, 0, 0, 0);
  }
}

void cassidy::Model::release(VkDevice device, VmaAllocator allocator)
{
  for (const auto& mesh : m_meshes)
  {
    mesh.release(device, allocator);
  }
}

void cassidy::Model::loadModel(const std::string& filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, aiPostProcessSteps additionalSteps)
{
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(MESH_ABS_FILEPATH + filepath,
    aiProcess_Triangulate |
    aiProcess_CalcTangentSpace |
    aiProcess_GenSmoothNormals |
    additionalSteps);

  if (!scene)
  {
    std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
    return;
  }

  std::string directory = filepath.substr(0, filepath.find_last_of('/') + 1);

  std::cout << "Found " << scene->mNumMaterials << " materials on model!" << std::endl;

  BuiltMaterials builtMaterials;

  processSceneNode(scene->mRootNode, scene, builtMaterials, directory, rendererRef);

  std::cout << "Successfully loaded mesh " << filepath << "!" << std::endl;
}

// Used for single-mesh models which have their vertices directly set by an array.
void cassidy::Model::setVertices(const std::vector<Vertex>& vertices)
{
  if (m_meshes.empty())
  {
    m_meshes.emplace_back(Mesh());
  }
  m_meshes[0].setVertices(vertices);
}

void cassidy::Model::setIndices(const std::vector<uint32_t>& indices)
{
  if (m_meshes.empty())
  {
    m_meshes.emplace_back(Mesh());
  }
  m_meshes[0].setIndices(indices);
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

    AllocatedBuffer stagingBuffer = {};

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

    AllocatedBuffer newBuffer = {};

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

void cassidy::Model::processSceneNode(aiNode* node, const aiScene* scene, BuiltMaterials& builtMaterials, const std::string& directory, cassidy::Renderer* rendererRef)
{
  m_meshes.reserve(node->mNumMeshes);

  for (uint32_t i = 0; i < node->mNumMeshes; ++i)
  {
    const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    m_meshes.emplace_back(Mesh());
    m_meshes[m_meshes.size() - 1].processMesh(mesh);

    const uint32_t matIndex = mesh->mMaterialIndex;

    //if (builtMaterials.find(matIndex) != builtMaterials.end())
    //{
    //  m_meshes[i].setMaterial(builtMaterials.at(matIndex));
    //  continue;
    //}

    MaterialInfo matInfo = m_meshes[i].buildMaterialInfo(scene, matIndex, directory, rendererRef);

    const aiMaterial* currentMat = scene->mMaterials[matIndex];

    constexpr MaterialLibrary& matLibrary = cassidy::globals::g_resourceManager.materialLibrary;
    cassidy::Material* builtMaterial = matLibrary.buildMaterial(directory + std::string(currentMat->GetName().C_Str()), matInfo);
    m_meshes[i].setMaterial(builtMaterial);
    builtMaterials[matIndex] = builtMaterial;
  }

  // Recursively iterate over child nodes and their meshes:
  for (uint32_t i = 0; i < node->mNumChildren; ++i)
  {
    processSceneNode(node->mChildren[i], scene, builtMaterials, directory, rendererRef);
  }
}

void cassidy::Mesh::processMesh(const aiMesh* mesh)
{
  Vertex vertex;

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

cassidy::MaterialInfo cassidy::Mesh::buildMaterialInfo(const aiScene* scene, uint32_t matIndex, const std::string& texturesDirectory, cassidy::Renderer* rendererRef)
{
  const aiMaterial* currentMat = scene->mMaterials[matIndex];
  std::string debugName = texturesDirectory + currentMat->GetName().C_Str();
  std::cout << "\nMaterial: " << currentMat->GetName().C_Str() << std::endl;

  cassidy::MaterialInfo matInfo;

  for (uint8_t i = aiTextureType_DIFFUSE; i < aiTextureType_UNKNOWN; ++i)
  {
    cassidy::TextureType engineTexType;
    const aiTextureType type = static_cast<aiTextureType>(i);
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    const char* texType = "";
    aiString texFilename;

    // Update image format based on texture map type:
    switch (type)
    {
    case aiTextureType_DIFFUSE:
      texType = "\tAlbedo";
      engineTexType = cassidy::TextureType::ALBEDO;
      break;
    case aiTextureType_SPECULAR:
      texType = "\tSpecular";
      format = VK_FORMAT_R8_UNORM;
      engineTexType = cassidy::TextureType::SPECULAR;
      break;
    case aiTextureType_AMBIENT:
      texType = "\tAmbient";
      format = VK_FORMAT_R8_UNORM;
      engineTexType = cassidy::TextureType::AO;
      break;
    case aiTextureType_EMISSIVE:
      texType = "\tEmissive";
      engineTexType = cassidy::TextureType::EMISSIVE;
      break;
    case aiTextureType_HEIGHT:
      texType = "\tHeight";
      format = VK_FORMAT_R8_UNORM;
      break;
    case aiTextureType_NORMALS:
      texType = "\tNormal";
      format = VK_FORMAT_R8G8B8A8_UNORM;
      engineTexType = cassidy::TextureType::NORMAL;
      break;
    case aiTextureType_DISPLACEMENT:
      texType = "\tDisplacement";
      format = VK_FORMAT_R8_UNORM;
      break;
    case aiTextureType_METALNESS:
      texType = "\tMetallic";
      format = VK_FORMAT_R8_UNORM;
      engineTexType = cassidy::TextureType::METALLIC;
      break;
    case aiTextureType_AMBIENT_OCCLUSION:
      texType = "\tAO";
      format = VK_FORMAT_R8_UNORM;
      engineTexType = cassidy::TextureType::AO;
      break;
    case aiTextureType_LIGHTMAP:
      texType = "\tAO (lightmap)";
      format = VK_FORMAT_R8_UNORM;
      continue;
    case aiTextureType_BASE_COLOR:
      texType = "\tBase color";
      engineTexType = cassidy::TextureType::ALBEDO;
      break;
    case aiTextureType_DIFFUSE_ROUGHNESS:
      texType = "\tDiffuse-roughness";
      format = VK_FORMAT_R8G8_UNORM;
      engineTexType = cassidy::TextureType::SPECULAR;
      break;
    default:
      // Skip unneeded texture.
      continue;
    }

    if (scene->mMaterials[matIndex]->GetTexture(type, 0, &texFilename) == aiReturn::aiReturn_SUCCESS)
    {
      const char* texName = texFilename.C_Str();
      std::cout << texType << ": " << texName;

      constexpr TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;
      cassidy::Texture* loadedTexture = texLibrary.loadTexture(MESH_ABS_FILEPATH + texturesDirectory + texName, format, VK_TRUE);

      if (!loadedTexture)
      {
        // Attempt to find embedded version of texture:
        if (scene->HasTextures())
        {
          const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texName);

          // When loading embedded textures with ASSIMP, if mHeight is 0 then the texture is in a 
          // compressed format (e.g. JPEG), and the texture size is mWidth instead of mWidth * mHeight.
          const VkExtent2D extent = embeddedTex->mHeight == 0 ?
          VkExtent2D{
            static_cast<uint32_t>(std::ceil(std::sqrt(embeddedTex->mWidth))),
            static_cast<uint32_t>(std::ceil(std::sqrt(embeddedTex->mWidth))),
          } :
          VkExtent2D{
            std::max(embeddedTex->mWidth, 1U),
            std::max(embeddedTex->mHeight, 1U),
          };

          size_t texSize = embeddedTex->mHeight == 0 ? 
            extent.width * extent.width :
            sizeof(aiTexel) * extent.width * extent.height;

          cassidy::Texture engineTex;
          VmaAllocator allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
          if (engineTex.create(reinterpret_cast<unsigned char*>(embeddedTex->pcData), texSize, extent,
            allocator, rendererRef, VK_FORMAT_R8_UNORM, VK_TRUE))
          {
            const std::string& name = MESH_ABS_FILEPATH + texturesDirectory + texName;

            texLibrary.registerTexture(name, engineTex);
            matInfo.attachTexture(texLibrary.getTexture(name), engineTexType);
          }
        }

        // Fallback to default texture based on type:
        cassidy::Texture* fallback = texLibrary.getFallbackTexture(engineTexType);
        std::cout << "\t(CASSIDY ERROR: could not load texture!)";

        matInfo.attachTexture(fallback, engineTexType);
      }
      else if (matInfo.pbrTextures.find(engineTexType) == matInfo.pbrTextures.end())
      {
        matInfo.attachTexture(loadedTexture, engineTexType);
      }

      std::cout << std::endl;
    }
  }
  matInfo.debugName = debugName;
  return matInfo;
}

void cassidy::Mesh::release(VkDevice device, VmaAllocator allocator) const
{
  vmaDestroyBuffer(allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
  vmaDestroyBuffer(allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
}