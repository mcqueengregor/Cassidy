#include "PostProcessStack.h"
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/Initialisers.h>
#include <Core/ResourceManager.h>

void cassidy::PostProcessStack::release()
{
	VkDevice device = m_rendererRef->getLogicalDevice();
	VmaAllocator alloc = cassidy::globals::g_resourceManager.getVmaAllocator();

	CS_LOG_INFO("Releasing {0} effects from post processing stack...", m_postProcessStack.size());

	for (auto&& p : m_postProcessStack)
	{
		p.pipeline.release(device);
		
		for (uint8_t i = 0; i < p.resultsImages.size(); ++i)
		{
			vmaDestroyImage(alloc, p.resultsImages[i].image, p.resultsImages[i].allocation);
			vkDestroyImageView(device, p.resultsImages[i].view, nullptr);
		}
	}
	CS_LOG_INFO("Released all post processing effects!");
}

void cassidy::PostProcessStack::swap(size_t firstIndex, size_t secondIndex)
{
	if (firstIndex >= m_postProcessStack.size() || secondIndex >= m_postProcessStack.size()
		|| firstIndex == secondIndex)
	{
		CS_LOG_WARN("Invalid arguments for PostProcessStack::swap (size = {2}, arguments {0}, {1})",
			firstIndex, secondIndex, m_postProcessStack.size());
		return;
	}

	PostProcessResources temp = m_postProcessStack[firstIndex];
	m_postProcessStack[firstIndex] = m_postProcessStack[secondIndex];
	m_postProcessStack[secondIndex] = temp;

	CS_LOG_INFO("Swapped elements {0} and {1} of post process stack!", firstIndex, secondIndex);
}

void cassidy::PostProcessStack::recordCommands(VkCommandBuffer cmd, uint32_t frameIndex)
{
	if (m_postProcessStack.empty()) return;

	VkExtent2D imageExtent = m_rendererRef->getSwapchain().extent;

	for (std::vector<PostProcessResources>::iterator it = m_postProcessStack.begin(); it != m_postProcessStack.end(); ++it)
	{
		if (!(*it).isActive) continue;

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (*it).pipeline.getPipeline());
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (*it).pipeline.getLayout(), 
			0, 1, &(*it).descriptorSets[frameIndex], 0, nullptr);

		vkCmdDispatch(cmd, std::ceil(imageExtent.width / 16.0), std::ceil(imageExtent.height / 16.0), 1);
	}
}