#pragma once

#include <Core/Pipeline.h>

namespace cassidy {

	struct PostProcessResources
	{
		cassidy::Pipeline pipeline;
		std::vector<AllocatedImage> resultsImages;
		std::vector<VkDescriptorSet> descriptorSets;
		VkClearValue clearColour;

		bool isActive = true;
	};

	class PostProcessStack
	{
	public:

		inline void init(size_t startingSize, cassidy::Renderer* rendererRef) { m_postProcessStack.reserve(startingSize);  m_rendererRef = rendererRef; }
		void release();

		inline void push(const PostProcessResources& resources) { m_postProcessStack.emplace_back(resources); }
		inline void pop() { m_postProcessStack.pop_back(); }
		void swap(size_t firstIndex, size_t secondIndex);

		void recordCommands(VkCommandBuffer cmd, uint32_t frameIndex);

		const PostProcessResources& get(size_t index) { return m_postProcessStack[index]; }

	private:
		std::vector<PostProcessResources> m_postProcessStack;
		cassidy::Renderer* m_rendererRef;
	};
};