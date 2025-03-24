#pragma once
#include <vector>
#include "Utils/Types.h"
#include "Core/Pipeline.h"
#include <Core/Mesh.h>
#include <Core/Texture.h>

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
      vkDestroyImageView(device, depthImage.view, nullptr);
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

    static inline std::vector<const char*> INSTANCE_EXTENSIONS = {
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    static inline std::vector<VkDynamicState> DYNAMIC_STATES = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };

    // Getters/setters: ------------------------------------------------------------------------------------------
    inline VkPhysicalDevice           getPhysicalDevice()       { return m_physicalDevice; }
    inline VkDevice                   getLogicalDevice()        { return m_device; }
    inline Swapchain                  getSwapchain()            { return m_swapchain; }
    inline UploadContext&             getUploadContext()        { return m_uploadContext; }
    inline VkPhysicalDeviceProperties getPhysDeviceProperties() { return m_physicalDeviceProperties; }
    inline VmaAllocator&              getAllocator()            { return m_allocator; }

  private:
    void updateBuffers(const FrameData& currentFrameData);
    void recordEditorCommands(uint32_t imageIndex);
    void recordViewportCommands(uint32_t imageIndex);
    void createImGuiCommands(uint32_t imageIndex);
    void submitCommandBuffers(uint32_t imageIndex);

    AllocatedBuffer allocateVertexBuffer(const std::vector<Vertex>& vertices);
    AllocatedBuffer allocateBuffer(uint32_t allocSize, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlagBits allocFlags);

    void initMemoryAllocator();
    void initLogicalDevice();
    void initSwapchain();

    void initEditorImages();
    void initEditorRenderPass();
    void initEditorFramebuffers();
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

    void transitionSwapchainImages();

    // Inlined methods:
    inline FrameData& getCurrentFrameData() { return m_frameData[m_currentFrameIndex]; }

    cassidy::Engine* m_engineRef;

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
    PhongLightingPushConstants m_phongLightingPushConstants;
    FrameData m_frameData[FRAMES_IN_FLIGHT];
    AllocatedBuffer m_perObjectUniformBufferDynamic;

    // Meshes:
    Model m_triangleMesh;
    Model m_backpackMesh;
    Texture m_backpackAlbedo;
    Texture m_backpackSpecular;
    UploadContext m_uploadContext;

    // Object data:
    glm::vec3 m_objectRotation = glm::vec3(0.0f);

    // Samplers:
    VkSampler m_viewportSampler;

    // Descriptor objects:
    VkDescriptorSetLayout m_perPassSetLayout;
    VkDescriptorSetLayout m_perObjectSetLayout; // (Dynamic)
    VkDescriptorSetLayout m_perMaterialSetLayout;

    VkDescriptorSet m_imguiViewportSets[FRAMES_IN_FLIGHT];  // Used to display the renderer viewport via an ImGui image.

    // Command objects:
    VkCommandPool                 m_graphicsCommandPool;
    std::vector<VkCommandBuffer>  m_commandBuffers;

    // Engine editor images and render pass:
    std::vector<AllocatedImage> m_editorImages;
    VkRenderPass                m_editorRenderPass;
    std::vector<VkFramebuffer>  m_editorFramebuffers;

    // Synchronisation objects:
    std::vector<VkSemaphore>  m_imageAvailableSemaphores;
    std::vector<VkSemaphore>  m_renderFinishedSemaphores;
    std::vector<VkFence>      m_inFlightFences;

    // Memory allocator and allocated objects:
    VmaAllocator m_allocator;

    // Viewport rendering objects:
    std::vector<AllocatedImage>   m_viewportImages;
    AllocatedImage                m_viewportDepthImage;
    Pipeline                      m_viewportPipeline;
    VkRenderPass                  m_viewportRenderPass;
    VkCommandPool                 m_viewportCommandPool;
    std::vector<VkFramebuffer>    m_viewportFramebuffers;
    std::vector<VkCommandBuffer>  m_viewportCommandBuffers;
    std::vector<VkDescriptorSet>  m_viewportDescSets;

    VkRenderPass                  m_imguiRenderPass;

    // Misc.:
    DeletionQueue m_deletionQueue;
    uint32_t m_currentFrameIndex;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;

    // TODO: Temp, tidy with mesh abstraction!
    const std::vector<Vertex> triangleVertices =
    {
      {{ 0.0f,  0.5f, 0.0f},  {0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.0f},  {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{ 0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    };

    const std::vector<uint32_t> triangleIndices =
    {
      0, 1, 2,
    };
  };
}
