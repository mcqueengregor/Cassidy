#pragma once
#include <Core/Material.h>
#include <unordered_map>

class MaterialLibrary
{
public:
  MaterialLibrary(const MaterialLibrary&) = delete;

  static MaterialLibrary& get()
  {
    static MaterialLibrary s_instance;
    return s_instance;
  }

  static inline cassidy::Material* buildMaterial(const std::string& materialName, const cassidy::MaterialInfo& materialInfo) {
    return MaterialLibrary::get().buildMaterialImpl(materialName, materialInfo);
  }

  static inline const std::unordered_map<std::string, cassidy::Material>& getMaterialCache() {
    return MaterialLibrary::get().getMaterialCacheImpl();
  }

  static inline uint32_t getNumDuplicateMaterialBuildsPrevented() {
    return MaterialLibrary::get().getNumDuplicateMaterialBuildsPreventedImpl();
  }

private:
  MaterialLibrary() {}

  cassidy::Material* buildMaterialImpl(const std::string& materialName, const cassidy::MaterialInfo& materialInfo);
  const std::unordered_map<std::string, cassidy::Material>& getMaterialCacheImpl() { return m_materialCache;  }
  uint32_t getNumDuplicateMaterialBuildsPreventedImpl() { return m_numDuplicateMaterialBuildsPrevented; }

  // TODO: Change string key value to texture hash (SHA-1?)
  std::unordered_map<std::string, cassidy::Material> m_materialCache;

  uint32_t m_numDuplicateMaterialBuildsPrevented = 0; // TODO: Restrict this to debug build?
};