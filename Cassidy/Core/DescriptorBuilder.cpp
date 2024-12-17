#include "DescriptorBuilder.h"

#include <Utils/Initialisers.h>

cassidy::DescriptorBuilder cassidy::DescriptorBuilder::begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
{
  DescriptorBuilder builder;

  builder.m_allocator = allocator;
  builder.m_cache = layoutCache;

  return builder;
}

cassidy::DescriptorBuilder& cassidy::DescriptorBuilder::bindBuffer(uint32_t bindingIndex,
  VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
  VkDescriptorSetLayoutBinding newBinding = cassidy::init::descriptorSetLayoutBinding(bindingIndex, type, 1, stageFlags, nullptr);
  m_bindings.push_back(newBinding);

  // (dstSet is left undefined, as it is set during the building of the final descriptor set object)
  VkWriteDescriptorSet newWrite = cassidy::init::writeDescriptorSet(0, bindingIndex, type, 1, bufferInfo);
  m_writes.push_back(newWrite);

  return *this;
}

cassidy::DescriptorBuilder& cassidy::DescriptorBuilder::bindImage(uint32_t bindingIndex, 
  VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
  VkDescriptorSetLayoutBinding newBinding = cassidy::init::descriptorSetLayoutBinding(bindingIndex, type, 1, stageFlags, nullptr);
  m_bindings.push_back(newBinding);

  // (dstSet is left undefined, as it is set during the building of the final descriptor set object)
  VkWriteDescriptorSet newWrite = cassidy::init::writeDescriptorSet(0, bindingIndex, type, 1, imageInfo);
  m_writes.push_back(newWrite);

  return *this;
}

bool cassidy::DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
  VkDescriptorSetLayoutCreateInfo info = cassidy::init::descriptorSetLayoutCreateInfo(
    static_cast<uint32_t>(m_bindings.size()), m_bindings.data());

  if (m_allocator->allocate(&set, layout) != VK_SUCCESS)
    return false;

  for (VkWriteDescriptorSet& w : m_writes)
    w.dstSet = set;

  vkUpdateDescriptorSets(m_allocator->getDeviceRef(), static_cast<uint32_t>(m_writes.size()),
    m_writes.data(), 0, nullptr);

  return true;
}

bool cassidy::DescriptorBuilder::build(VkDescriptorSet& set)
{
  VkDescriptorSetLayout layout;
  return build(set, layout);
}

void cassidy::DescriptorAllocator::resetAllPools()
{
  for (const auto& pool : m_usedDescriptorPools)
    vkResetDescriptorPool(m_deviceRef, pool, 0);

  m_freeDescriptorPools = m_usedDescriptorPools;
  m_usedDescriptorPools.clear();
  m_currentPool = VK_NULL_HANDLE;
}

VkBool32 cassidy::DescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
  if (m_currentPool == VK_NULL_HANDLE)
  {
    m_currentPool = grabPool();
    m_usedDescriptorPools.push_back(m_currentPool);
  }

  VkDescriptorSetAllocateInfo info = cassidy::init::descriptorSetAllocateInfo(m_currentPool, 1, &layout);

  VkResult allocResult = vkAllocateDescriptorSets(m_deviceRef, &info, set);

  bool shouldReallocate = false;

  switch (allocResult)
  {
  case VK_SUCCESS:
    return true;

  case VK_ERROR_FRAGMENTED_POOL:
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    shouldReallocate = true;
    break;
    
  default:
    return false;
  }

  if (shouldReallocate)
  {
    m_currentPool = grabPool();
    m_usedDescriptorPools.push_back(m_currentPool);

    allocResult = vkAllocateDescriptorSets(m_deviceRef, &info, set);

    if (allocResult == VK_SUCCESS)
      return true;
  }
  return false;
}

void cassidy::DescriptorAllocator::release()
{
  for (auto pool : m_freeDescriptorPools)
    vkDestroyDescriptorPool(m_deviceRef, pool, nullptr);
  for (auto pool : m_usedDescriptorPools)
    vkDestroyDescriptorPool(m_deviceRef, pool, nullptr);
}

VkDescriptorPool cassidy::DescriptorAllocator::grabPool()
{
  if (m_freeDescriptorPools.size() > 0)
  {
    VkDescriptorPool freePool = m_freeDescriptorPools.back();
    m_freeDescriptorPools.pop_back();
    return freePool;
  }

  return createPool(1000, 0);
}

VkDescriptorPool cassidy::DescriptorAllocator::createPool(uint32_t count, VkDescriptorPoolCreateFlags flags)
{
  std::vector<VkDescriptorPoolSize> sizes;
  sizes.reserve(m_poolSizes.sizes.size());

  for (auto size : m_poolSizes.sizes)
    sizes.push_back({ size.first, uint32_t(size.second * count) });

  VkDescriptorPoolCreateInfo poolInfo = cassidy::init::descriptorPoolCreateInfo(
    static_cast<uint32_t>(sizes.size()), sizes.data(), count);

  VkDescriptorPool newPool;
  vkCreateDescriptorPool(m_deviceRef, &poolInfo, nullptr, &newPool);

  return newPool;
}