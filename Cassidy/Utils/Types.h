#pragma once

#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <optional>

// Deletion queue concept from https://vkguide.dev/docs/chapter-2/cleanup/:
struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void addFunction(std::function<void()>&& function)
  {
    deletors.push_back(function);
  }

  void execute()
  {
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
    {
      (*it)();
    }
    deletors.clear();
  }
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};