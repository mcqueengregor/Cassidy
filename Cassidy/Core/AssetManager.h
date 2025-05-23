#pragma once

#include <Core/MaterialLibrary.h>
#include <Core/TextureLibrary.h>

namespace cassidy
{
	class Renderer;

	class AssetManager
	{
		typedef std::unordered_map<std::string, cassidy::Model> LoadedModels;
	public:

		void init(VmaAllocator* allocatorRef, cassidy::Renderer* rendererRef);
		void release(VkDevice device, VmaAllocator allocator);

		TextureLibrary textureLibrary;
		MaterialLibrary materialLibrary;
		LoadedModels loadedModels;
	};

	namespace globals
	{
		AssetManager g_assetManager;
	}
};