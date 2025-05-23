#include "AssetManager.h"

void cassidy::AssetManager::init(VmaAllocator* allocatorRef, cassidy::Renderer* rendererRef)
{
	textureLibrary.init(allocatorRef, rendererRef);
}

void cassidy::AssetManager::release(VkDevice device, VmaAllocator allocator)
{
	materialLibrary.releaseAll(device, allocator);
	textureLibrary.releaseAll(device, allocator);
}
