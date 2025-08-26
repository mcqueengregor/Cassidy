#include "PostProcessStack.h"
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/Initialisers.h>

void cassidy::PostProcessStack::swap(size_t firstIndex, size_t secondIndex)
{
	if (firstIndex >= m_postProcessStack.size() || secondIndex >= m_postProcessStack.size()
		|| firstIndex == secondIndex)
	{
		CS_LOG_WARN("Invalid arguments for PostProcessStack::swap (size = {2}, arguments {0}, {1}",
			firstIndex, secondIndex, m_postProcessStack.size());
		return;
	}

	PostProcessResources temp = m_postProcessStack[firstIndex];
	m_postProcessStack[firstIndex] = m_postProcessStack[secondIndex];
	m_postProcessStack[secondIndex] = temp;

	CS_LOG_INFO("Swapped elements {0} and {1} of post process stack!", firstIndex, secondIndex);
}

void cassidy::PostProcessStack::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex)
{
	if (m_postProcessStack.empty()) return;

	VkRenderPass currentRenderPass = m_postProcessStack[0].renderPass;

	for (std::vector<PostProcessResources>::iterator it = m_postProcessStack.begin(); it != m_postProcessStack.end(); ++it)
	{
		if (!(*it).isActive) continue;

		if ((*it).renderPass != currentRenderPass)
		{
			vkCmdEndRenderPass(cmd);

			VkRenderPassBeginInfo beginInfo = cassidy::init::renderPassBeginInfo((*it).renderPass, (*it).framebuffers[imageIndex],
				{ 0, 0 }, m_rendererRef->getSwapchain().extent, 1, &(*it).clearColour);
			vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		
	}
	vkCmdEndRenderPass(cmd);
}
