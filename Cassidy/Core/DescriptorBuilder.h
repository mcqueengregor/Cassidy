#pragma once

#include <Utils/Types.h>

// Source: https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.h
namespace cassidy
{
  // Descriptor builder pattern class.
  // Concerned with caching common descriptor set layouts and managing descriptor pools,
  // creating descriptor sets used for materials and other rendering/compute data.
  class DescriptorBuilder
  {
  public:

    DescriptorBuilder(const DescriptorBuilder&) = delete;

    static DescriptorBuilder& get()
    {
      static DescriptorBuilder s_descriptorBuilder;
      return s_descriptorBuilder;
    }

    static inline void releaseAll(VkDevice device, VmaAllocator allocator) {
      DescriptorBuilder::get().releaseAllImpl(device, allocator);
    }

    DescriptorBuilder& bindBuffer(uint32_t bindingIndex, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    DescriptorBuilder& bindImage(uint32_t bindingIndex, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

  private:
    DescriptorBuilder() {}

    void releaseAllImpl(VkDevice device, VmaAllocator allocator);
  };

  // Container class for caching and reusing commonly-used descriptor set layouts:
  class DecriptorLayoutCache
  {
  public:

    struct DescriptorLayoutInfo
    {
      std::vector<VkDescriptorSetLayoutBinding> bindings;

      bool operator==(const DescriptorLayoutInfo& other) const
      {
        if (other.bindings.size() != bindings.size()) return false;
        
        for (uint16_t i = 0; i < bindings.size(); ++i)
        {
          if (other.bindings[i].binding != bindings[i].binding)                 return false;
          if (other.bindings[i].descriptorType != bindings[i].descriptorType)   return false;
          if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) return false;
          if (other.bindings[i].stageFlags != bindings[i].stageFlags)           return false;
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

  private:

    struct DescriptorLayoutHash
    {
      std::size_t operator()(const DescriptorLayoutInfo& k) const
      {
        return k.hash();
      }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
  };
  
  // 
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

    void      resetAllPools();
    VkBool32  allocate(VkDescriptorSet* allocatedSet, VkDescriptorSetLayout layout);
    void      release();

  private:
    VkDescriptorPool grabPool();
    VkDescriptorPool createPool();

    std::vector<VkDescriptorPool> m_usedDescriptorPools;
    std::vector<VkDescriptorPool> m_freeDescriptorPools;
  };
}