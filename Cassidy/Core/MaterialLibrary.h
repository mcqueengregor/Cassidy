#pragma once
#include <Core/Material.h>
#include <unordered_map>

class MaterialLibrary
{
public:
  MaterialLibrary() {}

  inline void releaseAll(VkDevice device, VmaAllocator allocator) { m_materialCache.clear(); }

  cassidy::Material* buildMaterial(const std::string& materialName, const cassidy::MaterialInfo& materialInfo);

  inline const std::unordered_map<std::string, cassidy::Material>& getMaterialCache() { return m_materialCache; }
  inline uint32_t getNumDuplicateMaterialBuildsPrevented() { return m_numDuplicateMaterialBuildsPrevented; }

private:
  // TODO: Change string key value to texture hash (SHA-1?)
  std::unordered_map<std::string, cassidy::Material> m_materialCache;

  uint32_t m_numDuplicateMaterialBuildsPrevented = 0; // TODO: Restrict this to debug build?
};