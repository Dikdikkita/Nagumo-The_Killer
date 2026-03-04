cmake_minimum_required(VERSION 3.10)
project(GameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Help CMake find MSYS2 libraries on Windows
if(WIN32 AND NOT MSVC)
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

# Find SDL2 and its components
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

# The core source files
set(SOURCES
    src/main.cpp
    src/Engine.cpp
    src/Player.cpp
    src/Map.cpp
    src/Raycaster.cpp
    src/Enemy.cpp
    src/Item.cpp
)

add_executable(GameEngine ${SOURCES})

# Copy assets folder to build directory
add_custom_command(TARGET GameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:GameEngine>/assets")

target_include_directories(GameEngine PRIVATE 
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(GameEngine PRIVATE 
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
)
