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
    MaterialLibrary::get().buildMaterialImpl(materialName, materialInfo);
  }

private:
  MaterialLibrary() {}

  cassidy::Material* buildMaterialImpl(const std::string& materialName, const cassidy::MaterialInfo& materialInfo);

  std::unordered_map<std::string, cassidy::Material, cassidy::MaterialInfoHash> m_materialCache;
};