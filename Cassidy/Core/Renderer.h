#pragma once
#include <vector>
#include "Utils/Types.h"

// Forward declarations:
class SDL_Window;

namespace cassidy
{
  struct Swapchain
  {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

    void release(VkDevice device)
    {
      for (const VkImageView& view : imageViews)
        vkDestroyImageView(device, view, nullptr);

      vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
  };

  class Engine;

  class Renderer
  {
  public:
    void init(Engine* engine);
    void draw();
    void release();

    static inline std::vector<const char*> VALIDATION_LAYERS = {
      "VK_LAYER_KHRONOS_validation",
    };

    static inline std::vector<const char*> DEVICE_EXTENSIONS = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };


  private:
    void initLogicalDevice();
    void initSwapchain();

    Engine* m_engineRef;

    DeletionQueue m_deletionQueue;

    Swapchain m_swapchain;

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    const uint8_t FRAMES_IN_FLIGHT = 2;
  };
}
