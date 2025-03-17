#include "MaterialLibrary.h"
#include <Core/TextureLibrary.h>  // (contains global texture samplers)
#include <Utils/DescriptorBuilder.h>
#include <Utils/Initialisers.h>
#include <iostream>

cassidy::Material* MaterialLibrary::buildMaterialImpl(const std::string& materialName, const cassidy::MaterialInfo& materialInfo)
{
  // If material already exists, return reference to it:
  if (m_materialCache.find(materialName) != m_materialCache.end())
  {
    std::cout << "Material " << materialName << " has already been built!" << std::endl;
    return &m_materialCache.at(materialName);
  }

  /*
    - As part of material creation, keep track of descriptor layouts used
    - Create pipelines after materials are created, use layouts list to create? (SPIR-V reflect would be perfect for this)
    - Some way to cache pipelines would make the above process even better (use desc set layouts and push constants to achieve 
      this, actual shader modules could be detached from pipeline cache)
    
    - Couple of options to proceed with material creation and use:
      - Could create pipelines on a per-material basis which use a specified set of shader modules and push constants, but that raises 
        the issue of per-pass and -object desc layout attaching (passing data around like that between renderer and model is a massive pain)
      - Could create pipelines at the current point of the renderer startup process and have materials keep track of their descriptor set layouts,
        
      Would materials ever have a different layout for set #2? They always only consist of combined image samplers for PBR textures, and if
      a mesh doesn't use a particular texture type (e.g. emissive, normal) then the fallback texture will fill in that gap automatically.
      
      Pipelines would require the material set layout, need a way to reference the correct layout from layout cache during pipeline creation
      Could have materials keep track of the descriptor set layout to be referenced?

  */

  cassidy::Material newMat;
  VkDescriptorSet matDescSet;
  newMat.setMatInfo(materialInfo);

  VkDescriptorImageInfo perMaterialAlbedoInfo = cassidy::init::descriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
    materialInfo.pbrTextures.at(cassidy::TextureType::ALBEDO)->getImageView(), cassidy::globals::m_linearTextureSampler);
  VkDescriptorImageInfo perMaterialSpecularInfo = cassidy::init::descriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
    materialInfo.pbrTextures.at(cassidy::TextureType::SPECULAR)->getImageView(), cassidy::globals::m_linearTextureSampler);
  VkDescriptorImageInfo perMaterialNormalInfo = cassidy::init::descriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    materialInfo.pbrTextures.at(cassidy::TextureType::NORMAL)->getImageView(), cassidy::globals::m_linearTextureSampler);

  cassidy::DescriptorBuilder::begin(&cassidy::globals::g_descAllocator, &cassidy::globals::g_descLayoutCache)
    .bindImage(0, &perMaterialAlbedoInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    .bindImage(1, &perMaterialSpecularInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    .bindImage(2, &perMaterialNormalInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    .build(matDescSet);

  newMat.setTextureDescSet(matDescSet);

  m_materialCache[std::string(materialInfo.debugName)] = newMat;

  return &m_materialCache.at(std::string(materialInfo.debugName));
}
