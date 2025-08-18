#pragma once
#include <Core/Texture.h>
#include <unordered_map>
#include <mutex>

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

    struct BlitCommandsList {
      VkCommandBuffer cmd;
      std::mutex recordingMutex;
      volatile uint8_t numTextureCommandsRecorded;  // TODO: Volatile necessary?
    };

    void init(VmaAllocator* allocatorRef, cassidy::Renderer* rendererRef);

    cassidy::Texture* loadTexture(const std::string& filepath, VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
    void registerTexture(const std::string& name, const cassidy::Texture& texture);

    void releaseAll(VkDevice device, VmaAllocator allocator);

    void generateFallbackTextures();
    cassidy::Texture* getFallbackTexture(cassidy::TextureType type);

    inline cassidy::Texture* getTexture(const std::string& name) { return &m_loadedTextures.at(name); }
    inline size_t getNumLoadedTextures() { return m_loadedTextures.size(); }
    inline const std::unordered_map<std::string, cassidy::Texture>& getTextureLibraryMap() { return m_loadedTextures; }
    inline BlitCommandsList& getBlitCommandsList() { return m_blitCommandsList; }

  private:
    std::unordered_map<std::string, cassidy::Texture> m_loadedTextures;
    VmaAllocator* m_allocatorRef;
    cassidy::Renderer* m_rendererRef;
    bool m_isInitialised = false;
    BlitCommandsList m_blitCommandsList;
  };
}