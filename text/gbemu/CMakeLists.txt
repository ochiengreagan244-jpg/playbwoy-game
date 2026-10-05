cmake_minimum_required(VERSION 3.16)

set(MSYS2_UCRT64_BIN "C:/msys64/ucrt64/bin")
set(CMAKE_C_COMPILER "${MSYS2_UCRT64_BIN}/gcc.exe" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${MSYS2_UCRT64_BIN}/g++.exe" CACHE FILEPATH "C++ compiler" FORCE)

project(gbemu CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 (installed via MSYS2 UCRT64)
find_package(SDL2 REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/memory.cpp
    src/cpu.cpp
    src/ppu.cpp
)

add_executable(gbemu ${SOURCES})

# Link SDL2 using the modern target approach
if(TARGET SDL2::SDL2main)
    target_link_libraries(gbemu PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(gbemu PRIVATE SDL2::SDL2)

# Copy SDL2.dll next to the executable (if needed)
if(WIN32)
    add_custom_command(TARGET gbemu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:gbemu>
    )
endif()

