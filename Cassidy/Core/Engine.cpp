#include "Engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <Vendor/imgui-docking/imgui.h>
#include <Vendor/imgui-docking/imgui_impl_sdl2.h>
#include <Vendor/imgui-docking/imgui_impl_vulkan.h>

#include <Core/PrimitiveMeshes.h>
#include <Vendor/assimp/include/assimp/postprocess.h>
#include <Core/ResourceManager.h>

#include <Core/Logger.h>

#include <vector>
#include <set>

#include "Utils/Initialisers.h"
#include "Utils/Helpers.h"

cassidy::Engine::Engine() :
  m_windowDimensions(glm::vec2(1920, 1080))
{
}

cassidy::Engine::Engine(glm::vec2 windowDimensions) :
  m_windowDimensions(windowDimensions)
{
}

cassidy::Engine::Engine(glm::uvec2 windowDimensions) :
  m_windowDimensions(windowDimensions)
{
}

void cassidy::Engine::init()
{
  CS_LOG_INFO("Initialising engine...");

  initInstance();
  initSurface();
  initDebugMessenger();

  m_camera.init(this);
  m_renderer.init(this);
  m_eventHandler.init();

  initDefaultModels();

  CS_LOG_INFO("Initialised engine!");
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
        CS_LOG_CRITICAL("Rebuilding swapchain (window resize)");
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
      buildGUI();

      m_renderer.draw();
    }
  }
}

void cassidy::Engine::release()
{
  m_renderer.release();
  m_deletionQueue.execute();

  CS_LOG_INFO("Engine shut down!");
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

    InputHandler::centreCursor(m_windowDimensions.x, m_windowDimensions.y);
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

void cassidy::Engine::buildGUI()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(m_window);
  ImGui::NewFrame();

  ImGui::FileBrowser& fileBrowser = m_renderer.getEditorFileBrowser();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Cassidy main");
    {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("Engine stats:");
      {
        ImGui::Text("Frametime: %fms", getDeltaTimeSecs() * 1000.0f);

        constexpr TextureLibrary& texLibrary = cassidy::globals::g_resourceManager.textureLibrary;
        constexpr MaterialLibrary& matLibrary = cassidy::globals::g_resourceManager.materialLibrary;
        constexpr ModelManager& modelManager = cassidy::globals::g_resourceManager.modelManager;
        const std::string texLibraryHeaderText = "Texture library size: " + std::to_string(texLibrary.getNumLoadedTextures());

        if (ImGui::TreeNode(texLibraryHeaderText.c_str()))
        {
          const auto& textureLibrary = texLibrary.getTextureLibraryMap();
          for (const auto& texture : textureLibrary)
          {
            std::string_view textureFilename = texture.first;
            size_t lastBackSlash = textureFilename.find_last_of('/');
            ++lastBackSlash;  // Advance one character forward to isolate the filename.
            std::string_view textureFilenameSub = textureFilename.substr(lastBackSlash, textureFilename.size() - lastBackSlash);
            ImGui::Text(textureFilenameSub.data());
          }
          ImGui::TreePop();
        }

        const std::string matLibraryHeaderText = "Material library size: " + std::to_string(matLibrary.getMaterialCache().size());

        if (ImGui::TreeNode(matLibraryHeaderText.c_str()))
        {
          const auto& materialCache = matLibrary.getMaterialCache();
          for (const auto& mat : materialCache)
          {
            std::string_view matName = mat.first;
            ImGui::Text(matName.data());
          }

          // TODO: Restrict this to debug build?
          ImGui::Text("(Num duplicate materials prevented: %i)", matLibrary.getNumDuplicateMaterialBuildsPrevented());
          ImGui::TreePop();
        }

        const std::string modelManagerHeaderText = std::to_string(modelManager.getNumLoadedModels()) + " loaded models:";

        if (ImGui::TreeNode(modelManagerHeaderText.c_str()))
        {
          const auto& loadedModels = modelManager.getLoadedModels();
          for (const auto& model : loadedModels)
          {
            std::string_view modelName = model.first;
            ImGui::Text(modelName.data());
          }
          ImGui::TreePop();
        }

        if (ImGui::BeginListBox("Loaded models"))
        {
          for (int8_t i = 0; i < (int8_t)modelManager.getModelsPtrTable().size(); ++i)
          {
            const bool isCurrentlySelected = i == m_uiContext.selectedModel;

            if (ImGui::Selectable(modelManager.getModelsPtrTable()[i]->getDebugName().data(), &isCurrentlySelected))
              m_uiContext.selectedModel = i;

            if (isCurrentlySelected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndListBox();
          ImGui::Text("Current model: %i", m_uiContext.selectedModel);
          ImGui::Text("Post process steps: %u", m_uiContext.importPostProcessSteps);
        }
      }

      //ImGui::Text("Directional light:");
      //ImGui::SliderInt("Current light index:", &m_currentLightIndex, 0, NUM_LIGHTS - 1);
      //ImGui::SliderFloat("Light pitch", &m_lightRotation[m_currentLightIndex].x, 0.0f, 360.0f);
      //ImGui::SliderFloat("Light yaw", &m_lightRotation[m_currentLightIndex].y, 0.0f, 360.0f);
      //ImGui::SliderFloat("Light roll", &m_lightRotation[m_currentLightIndex].z, 0.0f, 360.0f);
      //ImGui::SliderFloat("Ambient", &m_lightAmbient[m_currentLightIndex], 0.0f, 1.0f);

      //ImGui::SliderInt("Num active lights: ", &m_numActiveLights, 0, NUM_LIGHTS);

      //ImGui::Text("Object rotation:");
      //ImGui::SliderFloat("Object pitch", &m_objectRotation.x, 0.0f, 360.0f);
      //ImGui::SliderFloat("Object yaw", &m_objectRotation.y, 0.0f, 360.0f);
      //ImGui::SliderFloat("Object roll", &m_objectRotation.z, 0.0f, 360.0f);

      if (ImGui::Button("Load model"))
        fileBrowser.Open();
    }
    ImGui::End();

    fileBrowser.Display();

    if (fileBrowser.HasSelected())
    {
      constexpr cassidy::ModelManager& modelManager = cassidy::globals::g_resourceManager.modelManager;
      const std::string& selectedString = fileBrowser.GetSelected().generic_string();
      modelManager.loadModel(selectedString, &m_renderer, static_cast<aiPostProcessSteps>(m_uiContext.importPostProcessSteps));
      
      modelManager.getModel(selectedString)->allocateVertexBuffers(m_renderer.getUploadContext().uploadCommandBuffer,
        cassidy::globals::g_resourceManager.getVmaAllocator(), &m_renderer);
      modelManager.getModel(selectedString)->allocateIndexBuffers(m_renderer.getUploadContext().uploadCommandBuffer,
        cassidy::globals::g_resourceManager.getVmaAllocator(), &m_renderer);

      fileBrowser.ClearSelected();
    }

    if (ImGui::Begin("Import settings"))
    {
      bool flipUVs = (m_uiContext.importPostProcessSteps & aiProcess_FlipUVs) != (aiPostProcessSteps)0;
      if (ImGui::Checkbox("Flip UVs", &flipUVs))
        m_uiContext.importPostProcessSteps ^= aiProcess_FlipUVs;
    }
    ImGui::End();

    if (ImGui::Begin("Viewport"))
    {
      ImVec2 viewportSize = ImGui::GetContentRegionAvail();
      ImVec2 newViewportSize = viewportSize;

      const cassidy::Swapchain& swapchain = m_renderer.getSwapchain();

      float viewportAspect = viewportSize.y / viewportSize.x;
      float swapchainAspect = static_cast<float>(swapchain.extent.height) / static_cast<float>(swapchain.extent.width);

      // Force viewport aspect ratio to be equal to swapchain extent's aspect ratio:
      if (viewportAspect > swapchainAspect)
      {
        newViewportSize.y = std::floor(viewportSize.x * swapchainAspect);
      }

      viewportAspect = viewportSize.x / viewportSize.y;
      swapchainAspect = static_cast<float>(swapchain.extent.width) / static_cast<float>(swapchain.extent.height);

      if (viewportAspect > swapchainAspect)
      {
        newViewportSize.x = std::floor(viewportSize.y * swapchainAspect);
      }

      // Centre viewport image in window:
      ImVec2 cursorPos = ImGui::GetCursorPos();
      cursorPos.x += (viewportSize.x - newViewportSize.x) * 0.5f;
      cursorPos.y += (viewportSize.y - newViewportSize.y) * 0.5f;

      ImGui::SetCursorPos(cursorPos);
      ImGui::Image(m_renderer.getViewportDescSet(), newViewportSize);
    }
    ImGui::End();
  }

  ImGui::Render();
}

void cassidy::Engine::updateGlobalTimer()
{
  uint64_t currentTimeMs = SDL_GetTicks64();
}

void cassidy::Engine::initInstance()
{
  SDL_Init(SDL_INIT_VIDEO);

  VkApplicationInfo appInfo = cassidy::init::applicationInfo("Cassidy v0.0.4", 0, 0, 4, 0, VK_API_VERSION_1_3);
  m_window = SDL_CreateWindow(appInfo.pApplicationName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    m_windowDimensions.x, m_windowDimensions.y, SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE);

  m_deletionQueue.addFunction([=]() {
    SDL_DestroyWindow(m_window);
  });

  // Get extensions required by SDL:
  unsigned int numRequiredExtensions;
  if (SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, NULL) != SDL_TRUE)
  {
    CS_LOG_ERROR("SDL could not find number of required extensions!");
    return;
  }

  std::vector<const char*> extensionNames;
  extensionNames.resize(numRequiredExtensions + cassidy::Renderer::INSTANCE_EXTENSIONS.size());

  if (SDL_Vulkan_GetInstanceExtensions(m_window, &numRequiredExtensions, extensionNames.data()) == SDL_TRUE)
  {
    CS_LOG_INFO("{0} required extensions for SDL:", numRequiredExtensions);
    for (uint8_t i = 0; i < numRequiredExtensions; ++i)
    {
      CS_LOG_INFO("\t{0}", extensionNames[i]);
    }

    CS_LOG_INFO("{0} required extensions for engine instance:", cassidy::Renderer::INSTANCE_EXTENSIONS.size());
    for (size_t i = 0; i < cassidy::Renderer::INSTANCE_EXTENSIONS.size(); ++i)
    {
      CS_LOG_INFO("\t{0}", cassidy::Renderer::INSTANCE_EXTENSIONS[i]);
    }

    memcpy(extensionNames.data() + numRequiredExtensions,
      cassidy::Renderer::INSTANCE_EXTENSIONS.data(),
      cassidy::Renderer::INSTANCE_EXTENSIONS.size() * sizeof(char*));
  }
  else
  {
    CS_LOG_ERROR("SDL could not return all required extensions!");
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

  VkInstanceCreateInfo instanceCreateInfo = cassidy::init::instanceCreateInfo(&appInfo, 
    static_cast<uint32_t>(extensionNames.size()), extensionNames.data(),
    static_cast<uint32_t>(validationLayers.size()), validationLayers.data(), 
    &debugMessengerInfo);

  const VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

  if (result == VK_SUCCESS)
    CS_LOG_INFO("Successfully created Vulkan instance!");
  else
  {
    CS_LOG_ERROR("Failed to create Vulkan instance!");
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

void cassidy::Engine::initDebugMessenger()
{
  VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback,
  };
  VkResult debugUtilsResult = createDebugUtilsMessengerEXT(m_instance, &debugInfo, nullptr, &m_debugMessenger);

  if (debugUtilsResult != VK_SUCCESS)
  {
    CS_LOG_ERROR("Failed to create debug messenger!");
    return;
  }
  CS_LOG_INFO("Successfully created debug messenger!");

  m_deletionQueue.addFunction([=]() {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      m_instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func)
      func(m_instance, m_debugMessenger, nullptr);
    });
}

void cassidy::Engine::initDefaultModels()
{
  cassidy::Model triangleMesh;
  triangleMesh.setVertices(TRIANGLE_VERTEX.data(), TRIANGLE_VERTEX.size());
  triangleMesh.setIndices(TRIANGLE_INDEX.data(), TRIANGLE_INDEX.size());
  triangleMesh.setDebugName("Primitives/Triangle");

  constexpr cassidy::ModelManager& modelManager =
    cassidy::globals::g_resourceManager.modelManager;
  modelManager.registerModel("Primitives/Triangle", triangleMesh);
  modelManager.loadModel(MESH_ABS_FILEPATH + std::string("Helmet/DamagedHelmet.gltf"), &m_renderer, aiProcess_FlipUVs);

  const VmaAllocator& allocator = cassidy::globals::g_resourceManager.getVmaAllocator();
  modelManager.allocateBuffers(m_renderer.getUploadContext().uploadCommandBuffer, allocator, &m_renderer);

  CS_LOG_INFO("Initialised default models!");
}