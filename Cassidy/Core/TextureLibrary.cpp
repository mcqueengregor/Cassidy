#include "TextureLibrary.h"
#include <iostream>

#define FALLBACK_TEXTURE_PREFIX "Fallback/"

void TextureLibrary::initImpl(VmaAllocator allocator, cassidy::Renderer* rendererRef)
{
  generateFallbackTextures();
}

cassidy::Texture* TextureLibrary::loadTextureImpl(std::string filepath, VmaAllocator allocator, cassidy::Renderer* rendererRef,
  VkFormat format, VkBool32 shouldGenMipmaps)
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

void TextureLibrary::generateFallbackTexturesImpl()
{
  float magentaColour[] = { 1.0f, 0.0f, 1.0f, 1.0f };
  float normalColour[]  = { 0.5f, 0.5f, 1.0f, 1.0f };
  float blackColour[]   = { 0.0f, 0.0f, 0.0f, 1.0f };
  float whiteColour[]   = { 1.0f, 1.0f, 1.0f, 1.0f };

  const VkExtent2D fallbackTexDim = { 1, 1 };

  cassidy::Texture magentaTex;
  //magentaTex.create(reinterpret_cast<unsigned char*>(magentaColour), sizeof(float) * 4, fallbackTexDim,


  
}

cassidy::Texture* TextureLibrary::retrieveFallbackTextureImpl(cassidy::TextureType type)
{
  // Return default 1x1 white, black, magenta or (0.5, 0.5, 1.0) normal texture based on type:
  
  switch (type)
  {
  case cassidy::TextureType::DIFFUSE:
    return nullptr; // Magenta

  case cassidy::TextureType::NORMAL:
    return nullptr; // Normal

  case cassidy::TextureType::EMISSIVE:
  case cassidy::TextureType::ROUGHNESS:
  case cassidy::TextureType::METALLIC:
    return nullptr; // Black

  case cassidy::TextureType::AO:
    return nullptr; // White
  }
  return nullptr; // Magenta again?
}
