cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(silene VERSION 0.0.1)

set(SRC_FILES
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
)

add_executable(silene ${SRC_FILES})

target_compile_options(silene PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)

include(cmake/CPM.cmake)

CPMAddPackage("gh:merryhime/dynarmic#6.5.0")
CPMAddPackage("gh:gabime/spdlog@1.12.0")
CPMAddPackage("gh:CLIUtils/CLI11@2.4.2")

target_link_libraries(silene dynarmic)
target_link_libraries(silene spdlog)
target_link_libraries(silene CLI11::CLI11)
