cmake_minimum_required(VERSION 3.10)
project(NES LANGUAGES C)

# Set C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include(FetchContent)

#Fetch SDL3:
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.18
)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL3)

# Collect all .c files in current directory
file(GLOB C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
set(SOURCES ${C_SOURCES})

# Create the executable target
add_executable(nes ${SOURCES})

# Add include directories (external libraries)
target_include_directories(nes
    PRIVATE
        "${CMAKE_SOURCE_DIR}/Include"
)

target_link_libraries(nes PRIVATE SDL3::SDL3 )
# target_link_libraries(nes PRIVATE SDL3_image::SDL3_image)

# Have to copy all of the assets to the output directory
add_custom_command(TARGET nes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        $<TARGET_FILE_DIR:nes>/data
)
