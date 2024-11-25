## Build third-party libraries first:
add_subdirectory(Vendor)

add_executable(Cassidy
	main.cpp
	Core/Camera.h
	Core/Camera.cpp
	Core/Engine.h
	Core/Engine.cpp
	Core/EventHandler.h
	Core/EventHandler.cpp
	Core/InputHandler.h
	Core/InputHandler.cpp
	Core/Mesh.h
	Core/Mesh.cpp
	Core/Pipeline.h
	Core/Pipeline.cpp
	Core/Renderer.h
	Core/Renderer.cpp
	Core/Texture.h
	Core/Texture.cpp
	)
	
	## Setup includes to source dir (engine utils and third-party libraries):
	target_include_directories(Cassidy PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/Vendor	## Included to satisfy Assimp's #include <assimp/[file]> structure
		)
	target_link_libraries(Cassidy 
		glm
		vma
		stb_image
		assimp::assimp
		imgui
		)
	target_link_libraries(Cassidy
		##Vulkan::Vulkan	## VkGuide includes this link but CMake throws an error on this project?
		sdl2
		)

add_library(CassidyUtils STATIC
	Utils/GlobalTimer.h
	Utils/GlobalTimer.cpp
	Utils/Helpers.h
	Utils/Helpers.cpp
	Utils/Initialisers.h
	Utils/Initialisers.cpp
	Utils/Keycode.h
	Utils/MouseCode.h
	Utils/Types.h
	)
	
	target_include_directories(CassidyUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(CassidyUtils 
		glm 
		vma 
		stb_image 
		assimp 
		imgui
		)
	target_link_libraries(CassidyUtils
		##Vulkan::Vulkan
		sdl2
		)
	
##set_target_properties(CassidyUtils PROPERTIES FOLDER "Utils")