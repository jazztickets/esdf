cmake_minimum_required(VERSION 3.0)

# define constants
add_definitions(-DGAME_VERSION_STRING="1.0")
add_definitions("-DGLM_FORCE_RADIANS")
add_definitions("-DHAS_SOCKLEN_T")

# projects
project(esdf)

# add extra find modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
set(EXTRA_LIBRARIES)

# compiler specific rules
if(MSVC)
	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} winmm ws2_32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")

	# set output path
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)
	set(ZLIB_ROOT $ENV{ZLIB_ROOT})

else()

	# set default build type
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
	endif()

	# compiler flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11")

	# set output path
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

endif()

# find libraries
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
	message(SEND_ERROR "Could not find OpenGL!")
endif()

find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
	message(SEND_ERROR "Could not find SDL2!")
endif()

find_package(Lua 5.2 REQUIRED)
if(NOT LUA_FOUND)
	message(SEND_ERROR "Could not find Lua!")
endif()

find_package(Freetype REQUIRED)
if(NOT FREETYPE_FOUND)
	message(SEND_ERROR "Could not find Freetype 2!")
endif()

find_package(ZLIB REQUIRED)
if(NOT ZLIB_FOUND)
	message(SEND_ERROR "Could not find zlib!")
endif()

find_package(OpenAL REQUIRED)
if(NOT OPENAL_FOUND)
	message(SEND_ERROR "Could not find OpenAL!")
endif()

find_package(GLM REQUIRED)
if(NOT GLM_FOUND)
	message(SEND_ERROR "Could not find GLM!")
endif()

find_package(Vorbis REQUIRED)
if(NOT VORBIS_FOUND)
	message(SEND_ERROR "Could not find libvorbis or libvorbisfile!")
endif()

find_package(Ogg REQUIRED)
if(NOT OGG_FOUND)
	message(SEND_ERROR "Could not find libogg!")
endif()

# includes
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${OPENAL_INCLUDE_DIR})
include_directories(${VORBIS_INCLUDE_DIR})
include_directories(${OGG_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories("src/")

# add source files
file(GLOB MAIN_SRC "src/*.cpp" "src/*.h")
file(GLOB OBJECTS_SRC "src/objects/*.cpp" "src/objects/*.h")
file(GLOB UI_SRC "src/ui/*.cpp" "src/ui/*.h")
file(GLOB STATES_SRC "src/states/*.cpp" "src/states/*.h")
file(GLOB NETWORK_SRC "src/network/*.cpp" "src/network/*.h")
file(GLOB PNGLITE_SRC "src/pnglite/*.c" "src/pnglite/*.h")
file(GLOB ENET_SRC "src/enet/*.c" "src/enet/*.h")
file(GLOB ZLIB_SRC "src/zlib/*.cc" "src/zlib/*.h")

# add virtual folders
source_group(main FILES ${MAIN_SRC})
source_group(objects FILES ${OBJECTS_SRC})
source_group(ui FILES ${UI_SRC})
source_group(states FILES ${STATES_SRC})
source_group(network FILES ${NETWORK_SRC})
source_group(pnglite FILES ${PNGLITE_SRC})
source_group(enet FILES ${ENET_SRC})
source_group(zlib FILES ${ZLIB_SRC})

# build executable
add_executable(${CMAKE_PROJECT_NAME}
	${MAIN_SRC}
	${OBJECTS_SRC}
	${UI_SRC}
	${STATES_SRC}
	${NETWORK_SRC}
	${PNGLITE_SRC}
	${ENET_SRC}
	${ZLIB_SRC}
)

# link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
	${OPENGL_gl_LIBRARY}
	${FREETYPE_LIBRARIES}
	${SDL2_LIBRARY}
	${LUA_LIBRARIES}
	${OPENAL_LIBRARY}
	${ZLIB_LIBRARIES}
	${VORBIS_LIBRARIES}
	${OGG_LIBRARIES}
	${EXTRA_LIBRARIES}
)

# copy DLLs to working directory
if(MSVC)
else()
endif()
