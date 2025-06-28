#include "ModelManager.h"
#include <Core/ResourceManager.h>
#include <Core/Logger.h>

void cassidy::ModelManager::releaseAll(VkDevice device, VmaAllocator allocator)
{
	CS_LOG_INFO("Releasing {0} models...", m_loadedModels.size());
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
	m_modelsPtrTable.emplace_back(&m_loadedModels.at(filepath));
}

void cassidy::ModelManager::registerModel(const std::string& name, const cassidy::Model& model)
{
	if (m_loadedModels.find(name) != m_loadedModels.end())
	{
		CS_LOG_INFO("New model registered with model manager ({0})!", name);
		return;
	}
	m_loadedModels[name] = model;
	m_modelsPtrTable.emplace_back(&m_loadedModels.at(name));
}

void cassidy::ModelManager::allocateBuffers(VkCommandBuffer cmd, VmaAllocator allocator, cassidy::Renderer* rendererRef)
{
	CS_LOG_INFO("Allocating vertex and index buffers...");

	for (auto&& model : m_loadedModels)
	{
		if (model.second.getLoadResult() == LoadResult::NOT_FOUND)
			continue;

		model.second.allocateVertexBuffers(cmd, allocator, rendererRef);
		model.second.allocateIndexBuffers(cmd, allocator, rendererRef);
	}
}