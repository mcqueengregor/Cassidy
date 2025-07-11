#pragma once

#include <vulkan/vulkan.h>
#include <deque>
#include <array>
#include <functional>
#include <optional>
#include <string>

#include "Vendor/vma/vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <Vendor/glm/vec4.hpp>
#include <Vendor/glm/mat4x4.hpp>
#include <Vendor/glm/gtx/transform.hpp>

// Deletion queue concept from https://vkguide.dev/docs/chapter-2/cleanup/:
struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void addFunction(std::function<void()>&& function)
  {
    deletors.push_back(function);
  }

  void execute()
  {
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
    {
      (*it)();
    }
    deletors.clear();
  }
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct SpirvShaderCode
{
  size_t codeSize;
  uint32_t* codeBuffer;

  ~SpirvShaderCode() { delete[] codeBuffer; }
};

struct Vertex
{
  glm::vec3 position  = glm::vec3(0.0f);
  glm::vec2 uv        = glm::vec2(0.0f);
  glm::vec3 normal    = glm::vec3(0.0f);

  static VkVertexInputBindingDescription getBindingDesc()
  {
    VkVertexInputBindingDescription desc = {};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
  }

  // TODO: Make this dynamic, let the user define combinations of vec3s, vec4s, etc!
  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescs()
  {
    std::array<VkVertexInputAttributeDescription, 3> descs = {};

    // Posiiton:
    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[0].offset = offsetof(Vertex, position);

    // UV:
    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    descs[1].offset = offsetof(Vertex, uv);

    // Normals:
    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[2].offset = offsetof(Vertex, normal);

    return descs;
  }
};

enum class LoadResult : uint8_t
{
  SUCCESS = 0b0000'0001,
  NOT_FOUND = 0b000'0010,
};

// An image object allocated with Vulkan Memory Allocator
struct AllocatedImage
{
  VkImage image;
  VkImageView view;
  VmaAllocation allocation;
  VkFormat format;
};

// A buffer object allocated with Vulkan Memory Allocator
struct AllocatedBuffer
{
  VkBuffer buffer;
  VmaAllocation allocation;
};

struct DirectionalLight
{
  glm::vec4 directionWS;
  glm::vec3 colour;
  float ambient;
};

struct DefaultPushConstants
{
  glm::mat4 world;
  glm::mat4 viewProj;
};

// Layout of per-frame data accessible to shaders via uniform buffer:
struct PerFrameData
{
  glm::vec3 directionalLightDir;
};

// As above, but bound per-pass:
struct MatrixBufferData
{
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewProj;
  glm::mat4 invViewProj;
};

// Lighting information, bound per-pass:
constexpr uint32_t NUM_LIGHTS = 4;
struct LightBufferData
{
  uint32_t numActiveLights = 1;
  uint32_t padding[3];
  DirectionalLight dirLights[NUM_LIGHTS];
};

// As above, but bound per-mesh:
struct PerObjectData
{
  glm::mat4 world;
};

struct PhongLightingPushConstants
{
  DirectionalLight dirLight;
};

struct FrameData
{
  VkDescriptorSet perFrameSet;

  AllocatedBuffer perPassMatrixUniformBuffer;
  AllocatedBuffer perPassLightUniformBuffer;

  VkDescriptorSet perPassSet;

  VkDescriptorSet perObjectSet;
};

struct UploadContext
{
  VkCommandPool uploadCommandPool;
  VkCommandBuffer uploadCommandBuffer;
  VkFence uploadFence;
  VkQueue graphicsQueueRef;
};