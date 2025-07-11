#pragma once
#include <Core/Material.h>
#include <unordered_map>

namespace cassidy {
  class MaterialLibrary
  {
  public:
    MaterialLibrary() {}

    inline void releaseAll() { m_materialCache.clear(); }

    cassidy::Material* buildMaterial(const std::string& materialName, cassidy::MaterialInfo& materialInfo);

    void createErrorMaterial();
    cassidy::Material* getErrorMaterial();

    inline const std::unordered_map<std::string, cassidy::Material>& getMaterialCache() { return m_materialCache; }
    inline uint32_t getNumDuplicateMaterialBuildsPrevented() { return m_numDuplicateMaterialBuildsPrevented; }

  private:
    // TODO: Change string key value to texture hash (SHA-1?)
    std::unordered_map<std::string, cassidy::Material> m_materialCache;

    uint32_t m_numDuplicateMaterialBuildsPrevented = 0; // TODO: Restrict this to debug build?
  };
};