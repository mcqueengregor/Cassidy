#pragma once
#include <vector>
#include "Utils/Types.h"
#include "Core/Pipeline.h"
#include <Core/Mesh.h>
#include <Core/Texture.h>
#include <Core/DescriptorBuilder.h>

// Forward declarations:
struct SDL_Window;

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
    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

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
    void recordGuiCommands();
    void submitCommandBuffers(uint32_t imageIndex);
    
    AllocatedBuffer allocateVertexBuffer(const std::vector<Vertex>& vertices);
    AllocatedBuffer allocateBuffer(uint32_t allocSize, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlagBits allocFlags);

    void initMemoryAllocator();
    void initLogicalDevice();
    void initSwapchain();

    void initDefaultRenderPass();
    void initPipelines();
    void initSwapchainFramebuffers();
    void initCommandPool();
    void initCommandBuffers();
    void initSyncObjects();
    
    void initMeshes();

    void initDescriptorSets();
    
    void initVertexBuffers();
    void initIndexBuffers();
    void initUniformBuffers();

    void initImGui();
    void initViewportImages();
    void initViewportRenderPass();
    void initViewportCommandPool();
    void initViewportCommandBuffers();
    void initViewportFramebuffers();

    // Inlined methods:
    inline FrameData& getCurrentFrameData() { return m_frameData[m_currentFrameIndex]; }

    cassidy::Engine* m_engineRef;

    // Essential objects:
    Swapchain m_swapchain;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    // Pipelines and renderpasses:
    Pipeline m_helloTrianglePipeline;
    VkRenderPass m_backBufferRenderPass;

    // Rendering data:
    DefaultPushConstants m_matrixPushConstants;
    FrameData m_frameData[FRAMES_IN_FLIGHT];
    AllocatedBuffer m_perObjectUniformBufferDynamic;

    // Meshes:
    Model m_triangleMesh;
    Model m_backpackMesh;
    Texture m_backpackAlbedo;
    UploadContext m_uploadContext;

    // Samplers:
    VkSampler m_linearSampler;
    VkSampler m_viewportSampler;

    // Descriptor objects:
    VkDescriptorSetLayout m_perPassSetLayout;
    VkDescriptorSetLayout m_perObjectSetLayout; // (Dynamic)

    VkDescriptorSet m_imguiViewportSets[FRAMES_IN_FLIGHT];  // Used to display the renderer viewport via an ImGui image.

    cassidy::DescriptorAllocator m_descAllocator;
    cassidy::DescriptorLayoutCache m_descLayoutCache;

    // Command objects:
    VkCommandPool m_graphicsCommandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    // Synchronisation objects:
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    // Memory allocator and allocated objects:
    VmaAllocator m_allocator;

    // ImGui rendering objects:
    std::vector<AllocatedImage> m_viewportImages;
    std::vector<VkImageView> m_viewportImageViews;
    VkRenderPass m_viewportRenderPass;
    VkCommandPool m_viewportCommandPool;
    std::vector<VkFramebuffer> m_viewportFramebuffers;
    std::vector<VkCommandBuffer> m_viewportCommandBuffers;
    std::vector<VkDescriptorSet> m_viewportDescSets;

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
  };
}
