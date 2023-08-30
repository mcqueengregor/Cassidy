#include "Engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <set>

#include "Utils/Initialisers.h"
#include "Utils/Helpers.h"

void cassidy::Engine::init()
{
  std::cout << "Initialising engine..." << std::endl;

  initInstance();
  initSurface();

  m_renderer.init(this);
  std::cout << "Initialised engine!\n" << std::endl;
}

void cassidy::Engine::run()
{
  SDL_Event e;
  bool isRunning = true;

  while (isRunning)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_QUIT)
        isRunning = false;
      else if (e.type == SDL_KEYDOWN)
      {
        if (e.key.keysym.sym == SDLK_ESCAPE)
          isRunning = false;
      }

      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED)
      {
        m_renderer.rebuildSwapchain();
      }

    }
    // If window isn't minimised, run renderer:
    const uint32_t windowFlags = SDL_GetWindowFlags(m_window);
    if ((windowFlags & SDL_WINDOW_MINIMIZED) == 0)
      m_renderer.draw();
  }
}

void cassidy::Engine::release()
{
  m_renderer.release();
  m_deletionQueue.execute();

  std::cout << "Engine shut down!" << std::endl;
}

void cassidy::Engine::initInstance()
{
  SDL_Init(SDL_INIT_VIDEO);

  VkApplicationInfo appInfo = cassidy::init::applicationInfo("Cassidy v0.0.1", 0, 0, 1, 0, VK_API_VERSION_1_3);
  m_window = SDL_CreateWindow(appInfo.pApplicationName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    1920, 1080, SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE);

  m_deletionQueue.addFunction([=]() {
    SDL_DestroyWindow(m_window);
  });

  // Get extensions required by SDL:
  unsigned int numRequiredExtensions;
  SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, NULL);
  const char** extensionNames = new const char* [sizeof(char*) * numRequiredExtensions];
  const SDL_bool extResult = SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, extensionNames);

  if (extResult == SDL_TRUE)
  {
    std::cout << numRequiredExtensions << " required extensions for SDL: " << std::endl;
    for (uint8_t i = 0; i < numRequiredExtensions; ++i)
    {
      std::cout << extensionNames[i] << std::endl;
    }
    std::cout << std::endl;
  }
  else
  {
    std::cout << "ERROR: SDL could not return all required extensions!\n" << std::endl;
    return;
  }

  // Attach debug messenger to instance:
  VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = cassidy::init::debugMessengerCreateInfo(
    static_cast<VkDebugUtilsMessageSeverityFlagBitsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
    static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
    debugCallback);

  const auto& validationLayers = cassidy::Renderer::VALIDATION_LAYERS;

  VkInstanceCreateInfo instanceCreateInfo = cassidy::init::instanceCreateInfo(&appInfo, numRequiredExtensions,
    extensionNames, validationLayers.size(), validationLayers.data(), &debugMessengerInfo);

  const VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
  delete[] extensionNames;

  if (result == VK_SUCCESS)
    std::cout << "Successfully created Vulkan instance!\n" << std::endl;
  else
  {
    std::cout << "ERROR: Failed to create Vulkan instance!\n" << std::endl;
    return;
  }

  m_deletionQueue.addFunction([=]() {
    vkDestroyInstance(m_instance, nullptr);
  });
}

void cassidy::Engine::initSurface()
{
  SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

  m_deletionQueue.addFunction([=]() {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  });
}
