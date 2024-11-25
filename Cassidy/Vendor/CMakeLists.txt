find_package(Vulkan REQUIRED)

## Automated SDL2 package discovery (use manual filepath input for now)
## https://trenki2.github.io/blog/2017/06/02/using-sdl2-with-cmake/:
##find_package(SDL2 REQUIRED)
##include_directories(${SDL2_INCLUDE_DIRS})

## SDL2 link method copied from VkGuide:
add_library(sdl2 INTERFACE)
	set(sdl2_DIR "SDL_PATH" CACHE PATH "Path to SDL2")
	target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include)
	target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64)
	target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

add_library(glm INTERFACE)
	target_include_directories(glm INTERFACE glm)

add_library(vma INTERFACE)
	target_include_directories(vma INTERFACE vma)

add_library(stb_image INTERFACE)
	target_include_directories(stb_image INTERFACE stb_image)

add_library(assimp INTERFACE)
	target_include_directories(assimp INTERFACE assimp)

add_library(imgui STATIC)
	target_include_directories(imgui PUBLIC imgui)
	target_sources(imgui PRIVATE
		imgui-docking/imgui.h
		imgui-docking/imgui.cpp
		
		imgui-docking/imgui_demo.cpp
		imgui-docking/imgui_draw.cpp
		imgui-docking/imgui_widgets.cpp
		
		imgui-docking/imgui_impl_vulkan.cpp
		imgui-docking/imgui_impl_sdl2.cpp
		)
	
	target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)