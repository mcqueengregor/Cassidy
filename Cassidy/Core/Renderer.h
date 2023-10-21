#pragma once
#include <vector>
#include "Utils/Types.h"
#include "Core/Pipeline.h"

// Forward declarations:
class SDL_Window;

namespace cassidy
{
  class Engine;

  struct Swapchain
  {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    AllocatedImage depthImage;
    VkImageView depthView;
    VkFormat imageFormat;
    VkExtent2D extent;
    bool hasBeenBuilt = false;

    void release(VkDevice device, VmaAllocator allocator)
    {
      for (uint32_t i = 0; i < images.size(); ++i)
      {
        vkDestroyImageView(device, imageViews[i], nullptr);
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
      }

      vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
      vkDestroyImageView(device, depthView, nullptr);
      vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
  };

  constexpr uint8_t FRAMES_IN_FLIGHT = 2;

  class Renderer
  {
  public:
    void init(Engine* engine);
    void draw();
    void release();

    void rebuildSwapchain();
    void uploadBuffer(std::function<void(VkCommandBuffer cmd)>&& function);

    // Constant/static members and methods: ----------------------------------------------------------------------
    static inline std::vector<const char*> VALIDATION_LAYERS = {
      "VK_LAYER_KHRONOS_validation",
    };

    static inline std::vector<const char*> DEVICE_EXTENSIONS = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    static inline std::vector<VkDynamicState> DYNAMIC_STATES = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline VkPhysicalDevice getPhysicalDevice() { return m_physicalDevice; }
    inline VkDevice getLogicalDevice() { return m_device; }
    inline Swapchain getSwapchain() { return m_swapchain; }

  private:
    void updateBuffers(const FrameData& currentFrameData);
    void recordCommandBuffers(uint32_t imageIndex);
    void submitCommandBuffers(uint32_t imageIndex);
    
    AllocatedBuffer allocateVertexBuffer(const std::vector<Vertex>& vertices);
    AllocatedBuffer allocateBuffer(uint32_t allocSize, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlagBits allocFlags);

    void initMemoryAllocator();
    void initLogicalDevice();
    void initSwapchain();

    void initPipelines();
    void initSwapchainFramebuffers();
    void initCommandPool();
    void initCommandBuffers();
    void initSyncObjects();

    void initDescriptorSets();
    void initDescSetLayouts();
    void initDescriptorPool();
    
    void initVertexBuffers();
    void initUniformBuffers();

    // Inlined methods:
    inline FrameData& getCurrentFrameData() { return m_frameData[m_currentFrameIndex]; }

    Engine* m_engineRef;

    // Essential objects:
    Swapchain m_swapchain;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    // Pipelines:
    Pipeline m_helloTrianglePipeline;

    // Rendering data:
    DefaultPushConstants m_matrixPushConstants;
    FrameData m_frameData[FRAMES_IN_FLIGHT];
    AllocatedBuffer m_perObjectUniformBufferDynamic;

    // Descriptor objects:
    VkDescriptorPool m_descriptorPool;

    VkDescriptorSetLayout m_perPassSetLayout;
    VkDescriptorSetLayout m_perObjectSetLayout; // (Dynamic)

    // Command objects:
    VkCommandPool m_graphicsCommandPool;
    VkCommandPool m_uploadCommandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkCommandBuffer m_uploadCommandBuffer;

    // Synchronisation objects:
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    VkFence m_uploadFence;

    // Memory allocator and allocated objects:
    VmaAllocator m_allocator;

    // Misc.:
    DeletionQueue m_deletionQueue;
    uint32_t m_currentFrameIndex;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;

    // TODO: Temp, tidy with mesh abstraction!
    const std::vector<Vertex> triangleVertices =
    {
      {{ 0.0f,  0.5f, 0.0f},  {0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{ 0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    };
    AllocatedBuffer m_triangleVertexBuffer;
  };
}
