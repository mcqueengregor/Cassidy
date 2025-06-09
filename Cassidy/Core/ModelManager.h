#pragma once
#include <Core/Mesh.h>

enum aiPostProcessSteps;

namespace cassidy {
	class ModelManager
	{
	public:
		ModelManager() {}

		void releaseAll(VkDevice device, VmaAllocator allocator);

		void loadModel(const std::string& filepath, cassidy::Renderer* rendererRef, aiPostProcessSteps additionalSteps = (aiPostProcessSteps)0);
		void registerModel(const std::string& name, const cassidy::Model& model);

		void allocateBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef);

		inline Model* getModel(const std::string& name) { return m_loadedModels.find(name) != m_loadedModels.end() ? &m_loadedModels.at(name) : nullptr; }

		inline size_t getNumLoadedModels() { return m_loadedModels.size(); }
		inline auto& getLoadedModels() { return m_loadedModels; }

	private:
		std::unordered_map<std::string, cassidy::Model> m_loadedModels;
	};
};