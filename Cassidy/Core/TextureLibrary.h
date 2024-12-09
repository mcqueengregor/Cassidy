#pragma once
#include <Core/Texture.h>
#include <unordered_map>

class TextureLibrary
{
public:
  TextureLibrary(const TextureLibrary&) = delete;

  static TextureLibrary& get()
  {
    static TextureLibrary s_instance;
    return s_instance;
  }

  static inline cassidy::Texture* loadTexture(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef,
    VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE) {
    return TextureLibrary::get().loadTextureImpl(filepath, allocator, rendererRef, format, shouldGenMipmaps);
  }

  static inline void releaseAll(VkDevice device, VmaAllocator allocator) {
    TextureLibrary::get().releaseAllImpl(device, allocator);
  }

  static inline cassidy::Texture* retrieveFallbackTexture(cassidy::TextureType type) {
    return TextureLibrary::get().retrieveFallbackTextureImpl(type);  
  }

private:
  TextureLibrary() {}

  cassidy::Texture* loadTextureImpl(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
  void releaseAllImpl(VkDevice device, VmaAllocator allocator);
  cassidy::Texture* retrieveFallbackTextureImpl(cassidy::TextureType type);

  std::unordered_map<std::string, cassidy::Texture> m_loadedTextures;
};