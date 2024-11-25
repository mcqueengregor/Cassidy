cmake_minimum_required(VERSION 3.8)

project("Cassidy"
	VERSION 0.0.3
	LANGUAGES CXX
	)

set(CMAKE_CXX_STANDARD 17)

## Begin building Cassidy core and utilities code:
add_subdirectory(Cassidy)