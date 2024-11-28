## Build and create targets for third-party libraries first:
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

	Core/Material.h
	Core/Material.cpp
	Core/MaterialLibrary.h
	Core/MaterialLibrary.cpp

	Core/Texture.h
	Core/Texture.cpp
	Core/TextureLibrary.h
	Core/TextureLibrary.cpp

	Core/DescriptorBuilder.h
	Core/DescriptorBuilder.cpp

	Core/Pipeline.h
	Core/Pipeline.cpp
	Core/Renderer.h
	Core/Renderer.cpp
	)
	
	## Setup includes to source dir (engine utils and third-party libraries):
	target_include_directories(Cassidy PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/Vendor	## Included to satisfy Assimp's #include <assimp/[file]> structure
		${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL/include	## Included because SDL2 adds include to bin/Vendor for some reason.
		)
	target_link_libraries(Cassidy 
		glm
		vma
		stb_image
		assimp
		imgui
		)
		
	target_link_libraries(Cassidy
		##Vulkan::Vulkan	## VkGuide includes this link but on this project CMake throws an error?
		SDL2
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
	
	target_include_directories(CassidyUtils PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL/include	## Included because SDL2 adds include to bin/Vendor for some reason.
		)
	target_link_libraries(CassidyUtils 
		glm 
		vma 
		stb_image 
		assimp 
		imgui
		)
	target_link_libraries(CassidyUtils
		##Vulkan::Vulkan
		SDL2
		)

## Don't forget to give engine core access to utils functions:
target_link_libraries(Cassidy CassidyUtils)

## Automatically copy shared library files to executable directory:
## https://github.com/libsdl-org/SDL/issues/6399
if (WIN32)
    add_custom_command(
        TARGET Cassidy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different 
		"$<TARGET_FILE:SDL2::SDL2>" 	## Copy target file of SDL2...
		"$<TARGET_FILE_DIR:Cassidy>"	## ...and paste into built executable dir.
        VERBATIM
		)
	add_custom_command(
		TARGET Cassidy POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		"$<TARGET_FILE:assimp>"
		"$<TARGET_FILE_DIR:Cassidy>"
		VERBATIM
		)
endif()

##set_target_properties(CassidyUtils PROPERTIES FOLDER "Utils")