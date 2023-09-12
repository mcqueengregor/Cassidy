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
    Engine();
    Engine(glm::vec2 windowDimensions);

    void init();
    void run();
    void release();

  private:
    void initInstance();
    void initSurface();

    VkInstance m_instance;

    SDL_Window* m_window;
    VkSurfaceKHR m_surface;
    glm::uvec2 m_windowDimensions;

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
    inline glm::uvec2 getWindowDim()  { return m_windowDimensions; }
    inline VkInstance getInstance()   { return m_instance; }
    inline VkSurfaceKHR getSurface()  { return m_surface; }
  };
}
