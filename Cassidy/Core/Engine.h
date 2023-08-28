#pragma once
#include "Core/Renderer.h"
#include "Utils/Types.h"

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

    VkInstance m_instance;

    SDL_Window* m_window;
    VkSurfaceKHR m_surface;

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

  public:
    // Getters and setters: --------------------------------------------------------------------------------------
    inline SDL_Window* getWindow()    { return m_window; }
    inline VkInstance getInstance()   { return m_instance; }
    inline VkSurfaceKHR getSurface()  { return m_surface; }
  };
}
