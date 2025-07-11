#include "MaterialLibrary.h"
#include <Core/ResourceManager.h>   // (Texture library contains global texture samplers)
#include <Core/Logger.h>
#include <Utils/DescriptorBuilder.h>
#include <Utils/Initialisers.h>
#include <iostream>

#define ERROR_MAT_NAME std::string("Default/ErrorMat")

cassidy::Material* cassidy::MaterialLibrary::buildMaterial(const std::string& materialName, cassidy::MaterialInfo& materialInfo)
{
  // If material already exists, return reference to it:
  if (m_materialCache.find(materialName) != m_materialCache.end())
  {
    CS_LOG_WARN("Using cached material {0}", materialName);
    ++m_numDuplicateMaterialBuildsPrevented;
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

    Material set layout (in this version of the engine) will always be:
    - Albedo texture image view, linear sampler
    - Specular texture image view, linear sampler
    - Normal texture image view, linear sampler

    When the descriptor builder is making a descriptor set for any material, the layout used will be one of the cached layouts
    When creating the Phong shading pipeline, it needs the descriptor set layout for the material
    Material *could* have decscriptor set layout reference since only one actual built layout object will exist
    VkGuide's material system in the GPU-driven engine has a 'ShaderEffect' struct which contains four different desc set layouts anyway,
    these are used to build pipeline layouts that SPIR-V reflect consumes later down the line
    Then the ShaderEffect is used to build a 'ShaderPass', which contains a pointer to the original effect plus the built pipeline and 
    pipeline layout (latter is probably for contiguous memory usage between pipeline and layout since effect already has built layout)

    An 'EffectTemplate' is the master material, which materials used in draw calls are built from. The final material object contains the
    descriptor sets used in rendering, plus a pointer to the original EffectTemplate (used in == operator override)
    The material's rendering data is the descriptor sets used for each render pass type (VkGuide engine uses opaque, transparent and 
    directional light shadow), handles to the textures used by the material and a pointer to the shader parameters structure used by the material.

    Takeaways from VkGuide's method:
    - Pipelines are cached based on the underlying pipeline layout data (descriptor set layouts and push constants)
    - The above data is built from SPIR-V reflection(?) and passed to a pipeline builder which creates the actual pipeline in a shader pass object
    - The shader passes for each type of pass (opaque forward, transparent forward and shadow) are collected into a master/base material object 
      which is cached
    - A built material is created from the descriptor sets allocated with VkGuide's descriptor builder (layout cache used to reuse material set layout 
      created during pipeline creation, the layout is recreated when the material's images are bound to the descriptor builder) and the resulting
      descriptor set can be bound during draw call recording

    New system proposal:
    - Build pipelines after mesh materials have been built (material library is populated)
    - Create runtime pipeline cache so that duplicate pipelines aren't created, use descriptor set layout and pipeline 
      debug name (e.g. "texturedPBR", "toon shading") as key
      - Shader should determine descriptor set layout? (SPIR-V reflect)
        - Until this is possible the descriptor set layout HAS to be hard-coded to textured Phong lighting shader layout (#0 per-pass, #1 per-object, 
          #2 per-material, vert + frag push constants).
      - Material should keep pointer to original material belonging to mesh to easily revert back?
      - Once materials have been created (minus pipeline attachment), create textured Phong lighting pipeline, iterate through all materials built
        and attach pipeline to them.
    - When recording draw calls, 

    v0.0.3: Just leave pipeline creation as-is for now, bind pipeline manually during draw recording and add material binds as Mesh::bindMaterial()
  */

  constexpr cassidy::TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;
  
  for (uint8_t i = 0; i <= (uint8_t)(cassidy::TextureType::SPECULAR); ++i)
  {
    const cassidy::TextureType type = static_cast<cassidy::TextureType>(i);
    if (!materialInfo.hasTexture(type))
      materialInfo.attachTexture(texLibrary.getFallbackTexture(type), type);
  }

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

  m_materialCache[materialInfo.debugName] = newMat;

  return &m_materialCache.at(materialInfo.debugName);
}

void cassidy::MaterialLibrary::createErrorMaterial()
{
  if (m_materialCache.find(ERROR_MAT_NAME) != m_materialCache.end())
    return;

  constexpr cassidy::TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;

  cassidy::MaterialInfo errorMatInfo = cassidy::MaterialInfo();
  errorMatInfo.debugName = ERROR_MAT_NAME;

  buildMaterial(errorMatInfo.debugName, errorMatInfo);
  CS_LOG_INFO("Created debug error material!");
}

cassidy::Material* cassidy::MaterialLibrary::getErrorMaterial()
{
  return &m_materialCache.at(ERROR_MAT_NAME);
}
