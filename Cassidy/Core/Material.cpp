#include "Material.h"

void cassidy::Material::release(VkDevice device, VmaAllocator allocator)
{
  for (auto [key, val] : m_info.pbrTextures)
  {
    val->release(device, allocator);
  }
}

cassidy::Material& cassidy::Material::addTexture()
{
  return *this;
}

cassidy::Material& cassidy::Material::setPipeline(Pipeline* pipeline)
{
  m_pipeline = pipeline;
  return *this;
}

bool cassidy::MaterialInfo::operator==(const MaterialInfo& other) const
{
    return false;
}
