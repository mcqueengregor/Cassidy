#pragma once
#include <Core/Texture.h>
#include <unordered_map>

namespace cassidy
{
  namespace globals
  {
    inline VkSampler m_linearTextureSampler;
    inline VkSampler m_nearestTextureSampler;
  }

  class TextureLibrary
  {
  public:
    TextureLibrary() {}

    void init(VmaAllocator* allocatorRef, cassidy::Renderer* rendererRef);
     
    cassidy::Texture* loadTexture(const std::string& filepath, VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
    void registerTexture(const std::string& name, const cassidy::Texture& texture);

    void releaseAll(VkDevice device, VmaAllocator allocator);

    void generateFallbackTextures();
    cassidy::Texture* retrieveFallbackTexture(cassidy::TextureType type);

    inline cassidy::Texture* getTexture(const std::string& name) { return &m_loadedTextures.at(name); }
    inline size_t getNumLoadedTextures() { return m_loadedTextures.size(); }
    inline const std::unordered_map<std::string, cassidy::Texture>& getTextureLibraryMap() { return m_loadedTextures; }

  private:
    std::unordered_map<std::string, cassidy::Texture> m_loadedTextures;
    VmaAllocator* m_allocatorRef;
    cassidy::Renderer* m_rendererRef;
    bool m_isInitialised = false;
  };
}