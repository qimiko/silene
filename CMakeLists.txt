cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(silene VERSION 0.0.1)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	# use angle by default on macos
	set(USE_ANGLE ON)
endif()

option(USE_ANGLE "Uses ANGLE instead of the desktop's OpenGL" OFF)

include(cmake/CPM.cmake)

CPMAddPackage("gh:merryhime/dynarmic#6.5.0")
CPMAddPackage("gh:gabime/spdlog@1.12.0")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.2")

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
	src/main.cpp

	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp

	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

add_executable(silene ${SRC_FILES})

target_compile_options(silene PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)

target_link_libraries(silene dynarmic)
target_link_libraries(silene spdlog)
target_link_libraries(silene CLI11::CLI11)

target_include_directories(silene PRIVATE "${imgui_SOURCE_DIR}")
target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/glfw")

if (USE_ANGLE)
	target_compile_definitions(silene PRIVATE -DIMGUI_IMPL_OPENGL_ES2)
	target_compile_definitions(silene PRIVATE -DGLFW_INCLUDE_NONE)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_EGL)
	target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/angle")
else()
	find_package(OpenGL)
	target_include_directories(silene PRIVATE ${OPENGL_INCLUDE_DIRS})
	target_link_libraries(silene ${OPENGL_LIBRARIES})
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	target_link_libraries(silene "${CMAKE_SOURCE_DIR}/third_party/link/macos/libglfw.3.dylib")

	if (USE_ANGLE)
		target_link_libraries(silene "${CMAKE_SOURCE_DIR}/third_party/link/macos/libGLESv2.dylib")
	else()
		target_compile_definitions(silene PRIVATE -DGL_SILENCE_DEPRECATION)
	endif()

	target_link_libraries(silene "-framework ApplicationServices")
	find_library(APP_SERVICES_LIBRARY ApplicationServices)
	mark_as_advanced(APP_SERVICES_LIBRARY)
	target_link_libraries(silene ${APP_SERVICES_LIBRARY})
endif()
