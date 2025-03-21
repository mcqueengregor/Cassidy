#pragma once
#include <Utils/Types.h>

// Source:  https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
// .h:      https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.h
// .cpp:    https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.cpp
namespace cassidy
{
  class DescriptorLayoutCache;
  class DescriptorAllocator;

  // Descriptor builder pattern class.
  // Concerned with caching common descriptor set layouts and managing descriptor pools,
  // creating descriptor sets used for materials and other rendering/compute data.
  class DescriptorBuilder
  {
  public:
    static DescriptorBuilder begin(DescriptorAllocator* allocator, DescriptorLayoutCache* layoutCache);

    DescriptorBuilder& bindBuffer(uint32_t bindingIndex, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    DescriptorBuilder& bindImage(uint32_t bindingIndex, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
    bool build(VkDescriptorSet& set);

  private:
    std::vector<VkWriteDescriptorSet>         m_writes;
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    DescriptorAllocator*    m_allocator;
    DescriptorLayoutCache*  m_cache;
  };

  // Container class for caching descriptor set layouts to prevent duplicates:
  class DescriptorLayoutCache
  {
  public:
    void init(VkDevice device) { m_deviceRef = device; }
    void release();

    VkDescriptorSetLayout createDescLayout(VkDescriptorSetLayoutCreateInfo* layoutCreateInfo);

    struct DescriptorLayoutInfo
    {
      std::vector<VkDescriptorSetLayoutBinding> bindings;

      bool operator==(const DescriptorLayoutInfo& other) const
      {
        if (other.bindings.size() != bindings.size()) return false;
        
        for (uint16_t i = 0; i < bindings.size(); ++i)
        {
          if (other.bindings[i].binding         != bindings[i].binding)         return false;
          if (other.bindings[i].descriptorType  != bindings[i].descriptorType)  return false;
          if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) return false;
          if (other.bindings[i].stageFlags      != bindings[i].stageFlags)      return false;
        }
        return true;
      }

      size_t hash() const
      {
        size_t result = std::hash<size_t>()(bindings.size());

        for (const VkDescriptorSetLayoutBinding& b : bindings)
        {
          // Pack binding data into I64:
          size_t bindingHash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

          result ^= std::hash<size_t>()(bindingHash);
        }

        return result;
      }
    };

    struct DescriptorLayoutHash
    {
      std::size_t operator()(const DescriptorLayoutInfo& k) const
      {
        return k.hash();
      }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
    VkDevice m_deviceRef;
  };
  
  // Manages descriptor pools and allocates new ones when needed:
  class DescriptorAllocator
  {
  public:
    struct PoolSizes
    {
      // Set of descriptor pool sizes (second member used as scalar
      // for descriptorCount, grows as more pools are created):
      std::vector<std::pair<VkDescriptorType, float>> sizes =
      {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
      };
    };

    void      init(VkDevice device) { m_deviceRef = device; }
    void      resetAllPools();
    VkBool32  allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
    void      release();

    VkDevice getDeviceRef() { return m_deviceRef; }

  private:
    VkDescriptorPool grabPool();
    VkDescriptorPool createPool(uint32_t count, VkDescriptorPoolCreateFlags flags);;

    VkDevice m_deviceRef;

    VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
    PoolSizes m_poolSizes;

    std::vector<VkDescriptorPool> m_usedDescriptorPools;
    std::vector<VkDescriptorPool> m_freeDescriptorPools;
  };

  namespace globals
  {
    inline DescriptorAllocator g_descAllocator;
    inline DescriptorLayoutCache g_descLayoutCache;
  };
}