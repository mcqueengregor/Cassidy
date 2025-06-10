#pragma once
#include "Core/Renderer.h"
#include "Core/EventHandler.h"
#include "Core/Camera.h"
#include "Utils/GlobalTimer.h"
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
    Engine(glm::uvec2 windowDimensions);

    void init();
    void run();
    void release();

  private:
    void processInput();
    void update();

    void buildGUI();

    void updateGlobalTimer();
    void initInstance();
    void initSurface();
    void initDebugMessenger();

    inline VkResult createDebugUtilsMessengerEXT(
      VkInstance                                instance,
      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
      const VkAllocationCallbacks*              pAllocator,
      VkDebugUtilsMessengerEXT*                 pDebugMessenger)
    {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");

      if (func)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkInstance m_instance;

    SDL_Window* m_window;
    VkSurfaceKHR m_surface;
    VkDebugUtilsMessengerEXT m_debugMessenger;  // TODO: Move this and surface, instance and window to renderer class(?)
    glm::uvec2 m_windowDimensions;

    cassidy::Camera m_camera;

    cassidy::EventHandler m_eventHandler;
    cassidy::Renderer m_renderer;

    struct UIContext {
      int selectedModel = 0;
    } m_uiContext;

    DeletionQueue m_deletionQueue;

    // Constant/static members and methods: ----------------------------------------------------------------------
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT             messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void*                                       pUserData)
    {
      std::cout << "Validation layer (";
      
      if ((messageType | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0)
        std::cout << " GENERAL";
      if ((messageType | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
        std::cout << " INFO";
      if ((messageType | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
        std::cout << " WARNING";
      if ((messageType | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
        std::cout << " ERROR";

      std::cout << " ): " << pCallbackData->pMessage << "\n" << std::endl;
      return VK_FALSE;
    }

  public:
    // Getters and setters: --------------------------------------------------------------------------------------
    inline SDL_Window* getWindow()      { return m_window; }
    inline glm::uvec2 getWindowDim()    { return m_windowDimensions; }
    inline VkInstance getInstance()     { return m_instance; }
    inline VkSurfaceKHR getSurface()    { return m_surface; }
    inline cassidy::Camera& getCamera() { return m_camera; }
    inline double getDeltaTimeSecs()    { return GlobalTimer::deltaTime(); }
    inline UIContext getUIContext()     { return m_uiContext; }
  };
}
