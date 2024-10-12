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

if(NOT DEFINED SILENE_USE_EGL AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
	set(SILENE_USE_EGL ON)
endif()

option(SILENE_USE_EGL "Uses OpenGL ES instead of desktop OpenGL, if ANGLE is not enabled" OFF)

include(cmake/CPM.cmake)

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
	src/android-application.cpp
	src/zip-file.cpp

	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp

	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

if(NOT SILENE_USE_ANGLE AND NOT SILENE_USE_EGL)
	set(SRC_FILES ${SRC_FILES} third_party/src/glad/glad.c)
endif()

set(MZ_BZIP2 OFF)
set(MZ_LZMA OFF)
set(MZ_ZSTD OFF)
set(MZ_PKCRYPT OFF)
set(MZ_WZAES OFF)
set(MZ_ICONV OFF)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
	set(MZ_FETCH_LIBS ON)
	set(SRC_FILES ${SRC_FILES}
		src/android-window.cpp
		src/android-main.cpp

		${imgui_SOURCE_DIR}/backends/imgui_impl_android.cpp
	)

	add_library(silene SHARED ${SRC_FILES})
else()
	set(SRC_FILES ${SRC_FILES}
		src/glfw-window.cpp
		src/main.cpp

		${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	)

	add_executable(silene ${SRC_FILES})
endif()

set_property(TARGET silene PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

target_compile_options(silene PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)

CPMAddPackage("gh:qimiko/dynarmic#1f55a37")
CPMAddPackage("gh:gabime/spdlog@1.14.1")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.2")
CPMAddPackage("gh:zlib-ng/minizip-ng#4.0.7")
CPMAddPackage("gh:ToruNiina/toml11@4.2.0")

target_link_libraries(silene dynarmic)
target_link_libraries(silene spdlog)
target_link_libraries(silene CLI11::CLI11)
target_link_libraries(silene minizip)
target_link_libraries(silene toml11::toml11)

target_include_directories(silene PRIVATE "${imgui_SOURCE_DIR}")

if (SILENE_USE_ANGLE)
	target_compile_definitions(silene PRIVATE -DIMGUI_IMPL_OPENGL_ES2)
	target_compile_definitions(silene PRIVATE -DGLFW_INCLUDE_NONE)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_EGL)
	target_compile_definitions(silene PRIVATE -DSILENE_USE_ANGLE)

	target_include_directories(silene PRIVATE "${CMAKE_SOURCE_DIR}/third_party/include/angle")
elseif(SILENE_USE_EGL)
	if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
		find_package(OpenGL REQUIRED COMPONENTS GLES2)
		target_include_directories(silene PRIVATE OPENGL_EGL_INCLUDE_DIRS)
		target_link_libraries(silene OpenGL::GLES2)
	else()
		target_link_libraries(silene GLESv2)
	endif()

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
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
	find_package(game-activity REQUIRED CONFIG)
	target_link_libraries(silene game-activity::game-activity_static EGL GLESv2 android log)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")
endif()
