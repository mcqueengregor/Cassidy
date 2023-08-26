#pragma once
#include "Renderer.h"
#include "Types.h"

#include <vulkan/vulkan.h>
#include <iostream>


namespace cassidy
{
  class Engine
  {
  public:
    void init();
    void run();
    void release();

  private:
    void initInstance();
    void initSurface();
    void initDevice();

    VkInstance m_instance;

    SDL_Window* m_window;
    VkSurfaceKHR m_surface;

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    cassidy::Renderer m_renderer;

    DeletionQueue m_deletionQueue;

    // Constant/static members and methods: ----------------------------------------------------------------------
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData)
    {
      std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
      return VK_FALSE;
    }

    const std::vector<const char*> VALIDATION_LAYERS = {
      "VK_LAYER_KHRONOS_validation",
    };

    const std::vector<const char*> DEVICE_EXTENSIONS = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
  };
}
