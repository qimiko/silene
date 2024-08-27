cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(silene VERSION 0.0.1)

set(SRC_FILES
	src/tcp-server.cpp
	src/gdb-server.cpp
	src/elf.cpp
	src/paged-memory.cpp
	src/elf-loader.cpp
	src/libc-state.cpp
	src/jni.cpp
	src/syscall-handler.cpp
	src/android-environment.cpp
	src/main.cpp
)

add_executable(silene ${SRC_FILES})

include(cmake/CPM.cmake)

CPMAddPackage("gh:merryhime/dynarmic#6.5.0")
CPMAddPackage("gh:gabime/spdlog@1.12.0")

target_link_libraries(silene dynarmic)
target_link_libraries(silene spdlog)
