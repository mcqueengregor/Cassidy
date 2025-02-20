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

  static inline void init(VmaAllocator allocator, cassidy::Renderer* rendererRef) {
    initImpl(allocator, rendererRef);
  }

  static inline cassidy::Texture* loadTexture(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef,
    VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE) {
    return TextureLibrary::get().loadTextureImpl(filepath, allocator, rendererRef, format, shouldGenMipmaps);
  }

  static inline void releaseAll(VkDevice device, VmaAllocator allocator) {
    TextureLibrary::get().releaseAllImpl(device, allocator);
  }

  static inline void generateFallbackTextures() {
    return TextureLibrary::get().generateFallbackTexturesImpl();
  }

  static inline cassidy::Texture* retrieveFallbackTexture(cassidy::TextureType type) {
    return TextureLibrary::get().retrieveFallbackTextureImpl(type);  
  }
  
  static inline size_t getNumLoadedTextures() {
    return TextureLibrary::get().getNumLoadedTexturesImpl();
  }

  static inline const std::unordered_map<std::string, cassidy::Texture>& getTextureLibraryMap() {
    return TextureLibrary::get().getTextureLibraryMapImpl();
  }

private:
  TextureLibrary() {}

  void              initImpl(VmaAllocator allocator, cassidy::Renderer* rendererRef);
  cassidy::Texture* loadTextureImpl(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef, VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE);
  void              releaseAllImpl(VkDevice device, VmaAllocator allocator);
  void              generateFallbackTexturesImpl();
  cassidy::Texture* retrieveFallbackTextureImpl(cassidy::TextureType type);
  inline size_t     getNumLoadedTexturesImpl() { return m_loadedTextures.size(); }
  inline const std::unordered_map<std::string, cassidy::Texture>& getTextureLibraryMapImpl() { return m_loadedTextures; }

  std::unordered_map<std::string, cassidy::Texture> m_loadedTextures;
};