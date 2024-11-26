#include "TextureLibrary.h"
#include <iostream>

cassidy::Texture* TextureLibrary::loadTextureImpl(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef,
  VkFormat format, VkBool32 shouldGenMipmaps = VK_FALSE)
{
  // If the texture has already been loaded, return the already-existing version:
  if (m_loadedTextures.find(filepath) != m_loadedTextures.end())
  {
    std::cout << "Texture " << filepath << " has already been loaded into memory!" << std::endl;
    return &m_loadedTextures.at(filepath);
  }

  // Otherwise, load the texture and add it to the libary, if the file loading is successful:
  cassidy::Texture newTexture;
  if (newTexture.load(filepath, allocator, rendererRef, format, shouldGenMipmaps))
  {
    m_loadedTextures[filepath] = newTexture;
    return &m_loadedTextures.at(filepath);
  }

  return nullptr;
}

void TextureLibrary::releaseAllImpl(VkDevice device, VmaAllocator allocator)
{
  for (auto& [key, val] : m_loadedTextures)
  {
    val.release(device, allocator);
  }
}
