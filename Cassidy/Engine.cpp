#include "Engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <set>

#include "Initialisers.h"
#include "Helpers.h"

void cassidy::Engine::init()
{
  std::cout << "Initialising engine..." << std::endl;

  initInstance();
  initSurface();
  initDevice();

  m_renderer.init();
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
    }
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
    1920, 1080, SDL_WindowFlags::SDL_WINDOW_VULKAN);

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

  VkInstanceCreateInfo instanceCreateInfo = cassidy::init::instanceCreateInfo(&appInfo, numRequiredExtensions,
    extensionNames, VALIDATION_LAYERS.size(), VALIDATION_LAYERS.data(), &debugMessengerInfo);

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

void cassidy::Engine::initDevice()
{
  m_physicalDevice = cassidy::helper::pickPhysicalDevice(m_instance);
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    std::cout << "ERROR: Failed to find physical device!\n" << std::endl;
    return;
  }

  QueueFamilyIndices indices = cassidy::helper::findQueueFamilies(m_physicalDevice, m_surface);

  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueInfo = cassidy::init::deviceQueueCreateInfo(queueFamily, 1, &queuePriority);
    queueInfos.push_back(queueInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceInfo = cassidy::init::deviceCreateInfo(queueInfos.data(), queueInfos.size(),
    &deviceFeatures, DEVICE_EXTENSIONS.size(), DEVICE_EXTENSIONS.data(), VALIDATION_LAYERS.size(),
    VALIDATION_LAYERS.data());

  vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

  m_deletionQueue.addFunction([=]() {
    vkDestroyDevice(m_device, nullptr);
  });
}
