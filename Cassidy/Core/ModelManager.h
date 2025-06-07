#pragma once
#include <Core/Mesh.h>

enum aiPostProcessSteps;

namespace cassidy {
	class ModelManager
	{
	public:
		ModelManager() {}

		inline void releaseAll(VkDevice device, VmaAllocator allocator);

		void loadModel(const std::string& filepath, cassidy::Renderer* rendererRef, aiPostProcessSteps additionalSteps = (aiPostProcessSteps)0);
		void registerModel(const std::string& name, const cassidy::Model& model);

	private:
		std::unordered_map<std::string, cassidy::Model> m_loadedModels;
	};
};