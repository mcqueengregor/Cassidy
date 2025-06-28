#include "ResourceManager.h"
#include <Core/Engine.h>
#include <Core/Renderer.h>
#include <Core/Logger.h>
#include <Utils/DescriptorBuilder.h>

void cassidy::ResourceManager::init(cassidy::Renderer* rendererRef, cassidy::Engine* engineRef)
{
	m_rendererRef = rendererRef;
	initVmaAllocator(engineRef);

	cassidy::globals::g_descAllocator.init(rendererRef->getLogicalDevice());
	cassidy::globals::g_descLayoutCache.init(rendererRef->getLogicalDevice());

	textureLibrary.init(&m_allocator, rendererRef);
	materialLibrary.createErrorMaterial();

	CS_LOG_INFO("Resource manager initialised!");
}

void cassidy::ResourceManager::release(VkDevice device)
{
	CS_LOG_INFO("Releasing resource manager...");
	materialLibrary.releaseAll();
	textureLibrary.releaseAll(device, m_allocator);
	modelManager.releaseAll(device, m_allocator);

	vmaDestroyAllocator(m_allocator);
}

void cassidy::ResourceManager::initVmaAllocator(cassidy::Engine* engineRef)
{
	VmaAllocatorCreateInfo info = {};
	info.physicalDevice = m_rendererRef->getPhysicalDevice();
	info.device = m_rendererRef->getLogicalDevice();
	info.instance = engineRef->getInstance();

	vmaCreateAllocator(&info, &m_allocator);

	CS_LOG_INFO("Created memory allocator!");
}
