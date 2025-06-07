#include "ModelManager.h"
#include <Core/ResourceManager.h>

inline void cassidy::ModelManager::releaseAll(VkDevice device, VmaAllocator allocator)
{
	for (auto&& model : m_loadedModels)
	{
		model.second.release(device, allocator);
	}
}

void cassidy::ModelManager::loadModel(const std::string& filepath, cassidy::Renderer* rendererRef, aiPostProcessSteps additionalSteps)
{
	Model newModel;
	const VmaAllocator& alloc = cassidy::globals::g_resourceManager.getVmaAllocator();
	newModel.loadModel(filepath, alloc, rendererRef, additionalSteps);

	if (newModel.getLoadResult() == LoadResult::NOT_FOUND)
		return;

	m_loadedModels[filepath] = newModel;
}

void cassidy::ModelManager::registerModel(const std::string& name, const cassidy::Model& model)
{
	if (m_loadedModels.find(name) == m_loadedModels.end())
	{
		// TODO: Add verbose log here when logging is implemented!
		return;
	}
	m_loadedModels[name] = model;
}
