#include "TextureLibrary.h"
#include <iostream>

#define FALLBACK_TEXTURE_PREFIX std::string("Fallback/")

void TextureLibrary::initImpl(VmaAllocator* allocatorRef, cassidy::Renderer* rendererRef)
{
  m_allocatorRef = allocatorRef;
  m_rendererRef = rendererRef;

  generateFallbackTexturesImpl();

  m_isInitialised = true;
}

cassidy::Texture* TextureLibrary::loadTextureImpl(std::string filepath, VkFormat format, VkBool32 shouldGenMipmaps)
{
  // If the texture has already been loaded, return the already-existing version:
  if (m_loadedTextures.find(filepath) != m_loadedTextures.end())
  {
    std::cout << "Texture " << filepath << " has already been loaded into memory!" << std::endl;
    return &m_loadedTextures.at(filepath);
  }

  // Otherwise, load the texture and add it to the libary, if the file loading is successful:
  cassidy::Texture newTexture;
  if (newTexture.load(filepath, *m_allocatorRef, m_rendererRef, format, shouldGenMipmaps))
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
  m_loadedTextures.clear();
  m_isInitialised = false;
}

void TextureLibrary::generateFallbackTexturesImpl()
{
  if (m_isInitialised) return;

  unsigned char magentaColour[] = { 1.0f * 255, 0.0f * 255, 1.0f * 255, 1.0f * 255 };
  unsigned char normalColour[]  = { 0.5f * 255, 0.5f * 255, 1.0f * 255, 1.0f * 255 };
  unsigned char blackColour[]   = { 0.0f * 255, 0.0f * 255, 0.0f * 255, 1.0f * 255 };
  unsigned char whiteColour[]   = { 1.0f * 255, 1.0f * 255, 1.0f * 255, 1.0f * 255 };

  float testFloat = 1.0f;
  unsigned char testChar = testFloat;

  const size_t fallbackTexSize = sizeof(float) * 4;
  const VkExtent2D fallbackTexDim = { 1, 1 };

  cassidy::Texture magentaTex, normalTex, blackTex, whiteTex;
  magentaTex.create(reinterpret_cast<unsigned char*>(magentaColour), fallbackTexSize, fallbackTexDim,
    *m_allocatorRef, m_rendererRef, VK_FORMAT_R8G8B8A8_SRGB);
  normalTex.create(reinterpret_cast<unsigned char*>(normalColour), fallbackTexSize, fallbackTexDim,
    *m_allocatorRef, m_rendererRef, VK_FORMAT_R8G8B8A8_UNORM);
  blackTex.create(reinterpret_cast<unsigned char*>(blackColour), fallbackTexSize, fallbackTexDim,
    *m_allocatorRef, m_rendererRef, VK_FORMAT_R8G8B8A8_SRGB);
  whiteTex.create(reinterpret_cast<unsigned char*>(blackColour), fallbackTexSize, fallbackTexDim,
    *m_allocatorRef, m_rendererRef, VK_FORMAT_R8G8B8A8_SRGB);

  const std::string magentaKeyVal = FALLBACK_TEXTURE_PREFIX + std::string("magenta");
  const std::string normalKeyVal  = FALLBACK_TEXTURE_PREFIX + std::string("normal");
  const std::string blackKeyVal   = FALLBACK_TEXTURE_PREFIX + std::string("black");
  const std::string whiteKeyVal   = FALLBACK_TEXTURE_PREFIX + std::string("white");

  m_loadedTextures[magentaKeyVal] = magentaTex;
  m_loadedTextures[normalKeyVal]  = normalTex;
  m_loadedTextures[blackKeyVal]   = blackTex;
  m_loadedTextures[whiteKeyVal]   = whiteTex;
}

cassidy::Texture* TextureLibrary::retrieveFallbackTextureImpl(cassidy::TextureType type)
{
  // Return default 1x1 white, black, magenta or (0.5, 0.5, 1.0) normal texture based on type:
  
  switch (type)
  {
  case cassidy::TextureType::DIFFUSE:
    return &m_loadedTextures.at(FALLBACK_TEXTURE_PREFIX + std::string("magenta"));

  case cassidy::TextureType::NORMAL:
    return &m_loadedTextures.at(FALLBACK_TEXTURE_PREFIX + std::string("normal"));

  case cassidy::TextureType::EMISSIVE:
  case cassidy::TextureType::ROUGHNESS:
  case cassidy::TextureType::METALLIC:
    return &m_loadedTextures.at(FALLBACK_TEXTURE_PREFIX + std::string("black"));

  case cassidy::TextureType::AO:
    return &m_loadedTextures.at(FALLBACK_TEXTURE_PREFIX + std::string("white"));
  }
  std::cout << "CASSIDY ERROR: No suitable fallback texture for TextureType " <<
    static_cast<uint8_t>(type) << ", defaulting to magenta." << std::endl;
  return &m_loadedTextures.at(FALLBACK_TEXTURE_PREFIX + std::string("magenta"));
}
