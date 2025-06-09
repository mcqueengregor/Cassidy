#pragma once
#include <Core/MaterialLibrary.h>
#include <Core/TextureLibrary.h>
#include <Core/ModelManager.h>

namespace cassidy
{
	class Renderer;
	class Engine;

	class ResourceManager
	{
	public:
		void init(cassidy::Renderer* rendererRef, cassidy::Engine* engineRef);
		void release(VkDevice device);

		inline VmaAllocator getVmaAllocator() { return m_allocator; }

		TextureLibrary textureLibrary;
		MaterialLibrary materialLibrary;
		ModelManager modelManager;

	private:
		void initVmaAllocator(cassidy::Engine* engineRef);

		VmaAllocator m_allocator;
		cassidy::Renderer* m_rendererRef;
	};

	namespace globals
	{
		inline ResourceManager g_resourceManager;
	}
};