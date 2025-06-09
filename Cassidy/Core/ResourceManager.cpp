#include "ResourceManager.h"
#include <Core/Engine.h>
#include <Core/Renderer.h>

void cassidy::ResourceManager::init(cassidy::Renderer* rendererRef, cassidy::Engine* engineRef)
{
	m_rendererRef = rendererRef;
	initVmaAllocator(engineRef);

	textureLibrary.init(&m_allocator, rendererRef);
}

void cassidy::ResourceManager::release(VkDevice device)
{
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
}
