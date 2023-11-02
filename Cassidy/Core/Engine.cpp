#include "Engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_vulkan.h>

#include <vector>
#include <set>

#include "Utils/Initialisers.h"
#include "Utils/Helpers.h"
#include <iomanip>

cassidy::Engine::Engine() :
  m_windowDimensions(glm::vec2(1920, 1080))
{
}

cassidy::Engine::Engine(glm::vec2 windowDimensions) :
  m_windowDimensions(windowDimensions)
{
}

void cassidy::Engine::init()
{
  std::cout << "Initialising engine..." << std::endl;

  initInstance();
  initSurface();

  m_camera.init(this);
  m_renderer.init(this);
  m_eventHandler.init();

  std::cout << "Initialised engine!\n" << std::endl;
}

void cassidy::Engine::run()
{
  SDL_Event e;
  bool isRunning = true;

  while (isRunning)
  {
    InputHandler::flushDynamicMouseStates();

    while (SDL_PollEvent(&e))
    {
      ImGui_ImplSDL2_ProcessEvent(&e);
      m_eventHandler.processEvent(&e);

      if (e.type == SDL_QUIT)
        isRunning = false;

      // If window was resized, get renderer to rebuild its swapchain:
      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED)
      {
        m_renderer.rebuildSwapchain();
        m_camera.updateProj();
      }
    }

    // Update delta time and time since engine was initialised:
    GlobalTimer::updateGlobalTimer();

    processInput();

    if (InputHandler::isKeyPressed(KeyCode::KEYCODE_ESCAPE))
    {
      isRunning = false;
      continue;
    }

    update();

    // If window isn't minimised, run renderer:
    const uint32_t windowFlags = SDL_GetWindowFlags(m_window);
    if ((windowFlags & SDL_WINDOW_MINIMIZED) == 0)
    {
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplSDL2_NewFrame(m_window);
      ImGui::NewFrame();

      m_renderer.draw();
    }
  }
}

void cassidy::Engine::release()
{
  m_renderer.release();
  m_deletionQueue.execute();

  std::cout << "Engine shut down!" << std::endl;
}

void cassidy::Engine::processInput()
{
  // Log key and mouse state changes between this frame and the previous frame:
  InputHandler::updateKeyStates();
  InputHandler::updateMouseStates();

  if (InputHandler::isMouseButtonPressed(MouseCode::MOUSECODE_RIGHT))
  {
    InputHandler::hideCursor();
    InputHandler::logMousePosition();
  }
  else if (InputHandler::isMouseButtonReleased(MouseCode::MOUSECODE_RIGHT))
  {
    InputHandler::showCursor();
    InputHandler::moveMouseToLoggedPosition();
  }
  if (InputHandler::isMouseButtonHeld(MouseCode::MOUSECODE_RIGHT))
  {
    m_camera.increaseYaw(static_cast<float>(InputHandler::getCursorOffsetX()));
    m_camera.increasePitch(static_cast<float>(InputHandler::getCursorOffsetY()));

    SDL_WarpMouseInWindow(NULL, 
      static_cast<int>(m_windowDimensions.x / 2.0f), static_cast<int>(m_windowDimensions.y / 2.0f));
  }

  // WASD horizontal camera movement controls:
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_w))
  {
    m_camera.moveForward();
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_a))
  {
    m_camera.moveRight(-1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_s))
  {
    m_camera.moveForward(-1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_d))
  {
    m_camera.moveRight();
  }

  // Q/E vertical camera movement controls:
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_q))
  {
    m_camera.moveUp(-1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_e))
  {
    m_camera.moveUp();
  }

  // Arrow key camera rotation controls:
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_UP))
  {
    m_camera.increasePitch(1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_DOWN))
  {
    m_camera.increasePitch(-1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_LEFT))
  {
    m_camera.increaseYaw(-1.0f);
  }
  if (InputHandler::isKeyHeld(KeyCode::KEYCODE_RIGHT))
  {
    m_camera.increaseYaw(1.0f);
  }
}

void cassidy::Engine::update()
{
  m_camera.update();
}

void cassidy::Engine::updateGlobalTimer()
{
  uint64_t currentTimeMs = SDL_GetTicks64();
}

void cassidy::Engine::initInstance()
{
  SDL_Init(SDL_INIT_VIDEO);

  VkApplicationInfo appInfo = cassidy::init::applicationInfo("Cassidy v0.0.2", 0, 0, 2, 0, VK_API_VERSION_1_3);
  m_window = SDL_CreateWindow(appInfo.pApplicationName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    1920, 1080, SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE);

  m_deletionQueue.addFunction([=]() {
    SDL_DestroyWindow(m_window);
  });

  // Get extensions required by SDL:
  unsigned int numRequiredExtensions;
  if (SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, NULL) != SDL_TRUE)
  {
    std::cout << "ERROR: SDL couldn't find number of required extensions!\n";
    return;
  }

  const char** extensionNames = new const char* [sizeof(char*) * numRequiredExtensions];
  if (SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, extensionNames) == SDL_TRUE)
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
