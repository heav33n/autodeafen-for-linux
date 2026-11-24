cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(AutoDeafenForLinux VERSION 0.1.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/popups.cpp
    src/level_config.cpp
    src/auth.cpp
    src/discord_link.cpp
    src/discord_socket.cpp
)
target_link_libraries(${PROJECT_NAME} ws2_32)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Set GEODE_SDK to a Geode v6.0.0-beta.2 SDK")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
