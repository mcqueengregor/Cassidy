## Build third-party libraries first:
add_subdirectory(Vendor)

add_executable(cassidy
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

##set_target_properties(CassidyUtils PROPERTIES FOLDER "Utils")