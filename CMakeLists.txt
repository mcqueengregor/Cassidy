cmake_minimum_required(VERSION 3.8)

project("Cassidy"
	VERSION 0.0.3
	LANGUAGES CXX
	)

set(CMAKE_CXX_STANDARD 20)

## Begin building Cassidy core and utilities code:
add_subdirectory(Cassidy)

## Define absolute filepaths for meshes and shaders (folders in root directory):
target_compile_definitions(Cassidy PUBLIC MESH_ABS_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/Meshes/")
target_compile_definitions(Cassidy PUBLIC SHADER_ABS_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/")

message(STATUS "Mesh filepath: ${CMAKE_CURRENT_SOURCE_DIR}/Meshes/")
message(STATUS "Shaders filepath:${CMAKE_CURRENT_SOURCE_DIR}/Shaders/")