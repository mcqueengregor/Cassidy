find_package(Vulkan REQUIRED)

## https://trenki2.github.io/blog/2017/06/02/using-sdl2-with-cmake/:
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

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
	imgui/imgui.h
	imgui/imgui.cpp
	
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	
	imgui/imgui_impl_vulkan.cpp
	imgui/imgui_impl_sdl2.cpp
	)
	
target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)