#pragma once
#include <Utils/Types.h>
#include <Core/Pipeline.h>
#include <Core/Texture.h>

// Forward declarations:
enum aiTextureType;

namespace cassidy
{
  // TODO: Change texture pointer to texture hash (hash of file's absolute filepath, use SHA-1?)
  typedef std::unordered_map<cassidy::TextureType, cassidy::Texture*> PBRTextures;

  struct MaterialInfo
  {
    std::string debugName;
    PBRTextures pbrTextures;

    bool operator==(const MaterialInfo& other) const;

    // Source: https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/material_system.cpp
    size_t hash() const
    {
      size_t result = std::hash<std::string>()(debugName);

      for (auto& [key, val] : pbrTextures)
      {
        size_t textureHash =
          (std::hash<size_t>()((size_t)val->getImage() << 3) & (std::hash<size_t>()((size_t)val->getImageView())));
        result ^= std::hash<size_t>()(textureHash);
      }
      return result;
    }

    void attachTexture(cassidy::Texture* texture, cassidy::TextureType type)
    {
      if (pbrTextures.find(type) == pbrTextures.end())
      {
        pbrTextures[type] = texture;
      }
    }
  };
  struct MaterialInfoHash
  {
    size_t operator()(const MaterialInfo& info) const
    {
      return info.hash();
    }
  };

  class Material
  {
  public:
    Material()
      : m_textureDescriptorSet(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE) {}

    void release(VkDevice device, VmaAllocator allocator);

    Material& addTexture();
    Material& setPipeline(Pipeline* pipeline);

    Pipeline& getPipeline() { return *m_pipeline; }

  private:
    VkDescriptorSet m_textureDescriptorSet;
    Pipeline* m_pipeline;
    cassidy::MaterialInfo m_info;
  };
};
