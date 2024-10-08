cmake_minimum_required(VERSION 3.27.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(silene VERSION 0.0.1)

if(NOT DEFINED SILENE_USE_ANGLE AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	# use angle by default on macos
	set(SILENE_USE_ANGLE ON)
endif()

option(SILENE_USE_ANGLE "Uses ANGLE instead of the desktop's OpenGL" OFF)

option(SILENE_USE_EGL "Uses OpenGL ES instead of desktop OpenGL, if ANGLE is not enabled" OFF)

include(cmake/CPM.cmake)

CPMAddPackage("gh:qimiko/dynarmic#6.7.0")
CPMAddPackage("gh:gabime/spdlog@1.14.1")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.2")
CPMAddPackage("gh:zlib-ng/minizip-ng#4.0.7")

CPMAddPackage(
	NAME imgui
	GITHUB_REPOSITORY ocornut/imgui
	VERSION 1.91.2
	DOWNLOAD_ONLY YES
)

set(SRC_FILES
	src/kernel/fcntl.cpp
	src/libc/stdio.cpp
	src/tcp-server.cpp
	src/gdb-server.cpp
	src/elf.cpp
	src/paged-memory.cpp
	src/elf-loader.cpp
	src/libc-state.cpp
	src/jni.cpp
	src/syscall-handler.cpp
	src/android-environment.cpp
	src/android-coprocessor.cpp
	src/zip-file.cpp
	src/main.cpp

	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp

	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

if (NOT SILENE_USE_ANGLE AND NOT SILENE_USE_EGL)
	set(SRC_FILES ${SRC_FILES} third_party/src/glad/glad.c)
endif()

add_executable(silene ${SRC_FILES})

target_compile_options(silene PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)

target_link_libraries(silene dynarmic)
target_link_libraries(silene spdlog)
target_link_libraries(silene CLI11::CLI11)
target_link_libraries(silene minizip)

target_include_directories(silene PRIVATE "${imgui_SOURCE_DIR}")

if (SILENE_USE_ANGLE)
	target_compile_definitions(silene PRIVATE -DIMGUI_IMPL_OPENGL_ES2)
	target_compile_definitions(silene PRIVATE -DGLFW_INCLUDE_NONE)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_EGL)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_ANGLE)

	target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/angle")
elseif(SILENE_USE_EGL)
	find_package(OpenGL REQUIRED COMPONENTS GLES2)
	target_include_directories(silene PRIVATE OPENGL_EGL_INCLUDE_DIRS)
	target_link_libraries(silene OpenGL::GLES2)

	target_compile_definitions(silene PRIVATE -DIMGUI_IMPL_OPENGL_ES2)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_EGL)
else()
	target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/glad")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/glfw")
	target_link_libraries(silene "${CMAKE_SOURCE_DIR}/third_party/link/macos/libglfw.3.dylib")

	if(SILENE_USE_ANGLE)
		target_link_libraries(silene "${CMAKE_SOURCE_DIR}/third_party/link/macos/libGLESv2.dylib")
	elseif(SILENE_USE_EGL)
		message(FATAL_ERROR "EGL mode is not supported on macOS, please enable ANGLE instead!")
	else()
		target_compile_definitions(silene PRIVATE -DGL_SILENCE_DEPRECATION)
	endif()

	target_link_libraries(silene "-framework ApplicationServices")
	find_library(APP_SERVICES_LIBRARY ApplicationServices)
	mark_as_advanced(APP_SERVICES_LIBRARY)
	target_link_libraries(silene ${APP_SERVICES_LIBRARY})
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	find_package(glfw3 3.3 REQUIRED)
	target_include_directories(silene PRIVATE ${GLFW_INCLUDE_DIRS})
	target_link_libraries(silene glfw)
endif()
