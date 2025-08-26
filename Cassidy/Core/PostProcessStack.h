#pragma once

#include <Core/Pipeline.h>

namespace cassidy {

	class PostProcessStack
	{
	public:
		struct PostProcessResources
		{
			cassidy::Pipeline pipeline;
			std::vector<AllocatedImage> resultsImages;
			std::vector<VkDescriptorSet> descriptorSets;
			std::vector<VkFramebuffer> framebuffers;
			VkRenderPass renderPass;
			VkClearValue clearColour;

			bool isActive = true;
		};

		inline void init(cassidy::Renderer* rendererRef) { m_rendererRef = rendererRef; }

		inline void push(const PostProcessResources& resources) { m_postProcessStack.emplace_back(resources); }
		inline void pop()																				{ m_postProcessStack.pop_back(); }
		void swap(size_t firstIndex, size_t secondIndex);

		void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex);

	private:
		std::vector<PostProcessResources> m_postProcessStack;
		cassidy::Renderer* m_rendererRef;
	};
};