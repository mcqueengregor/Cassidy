#include "MaterialLibrary.h"
#include <iostream>

cassidy::Material* MaterialLibrary::buildMaterialImpl(const std::string& materialName, const cassidy::MaterialInfo& materialInfo)
{
  // If material already exists, return reference to it:
  if (m_materialCache.find(materialName) != m_materialCache.end())
  {
    std::cout << "Material " << materialName << " has already been built!" << std::endl;
    return &m_materialCache.at(materialName);
  }

  cassidy::Material newMat;

  /* 
    - Get descriptor layout from descriptor builder layout cache
    - Descriptor builder creates descriptor set using available descriptor pool, descriptor layout and material info
  */
  return newMat;
}
