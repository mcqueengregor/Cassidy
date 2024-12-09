#include "DescriptorBuilder.h"

#include <Utils/Initialisers.h>

cassidy::DescriptorBuilder& cassidy::DescriptorBuilder::bindBuffer(uint32_t bindingIndex, 
  VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
  
  return *this;
}

cassidy::DescriptorBuilder& cassidy::DescriptorBuilder::bindImage(uint32_t bindingIndex, 
  VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
  return *this;
}

void cassidy::DescriptorBuilder::releaseAllImpl(VkDevice device, VmaAllocator allocator)
{
}

void cassidy::DescriptorAllocator::resetAllPools()
{
  //for (const auto& pool : m_usedDescriptorPools)
  //  vkResetDescriptorPool(, pool, )
}

VkBool32 cassidy::DescriptorAllocator::allocate(VkDescriptorSet* allocatedSet, VkDescriptorSetLayout layout)
{
  return VkBool32();
}

void cassidy::DescriptorAllocator::release()
{
}

VkDescriptorPool cassidy::DescriptorAllocator::grabPool()
{
  return VkDescriptorPool();
}

VkDescriptorPool cassidy::DescriptorAllocator::createPool()
{
  return VkDescriptorPool();
}
