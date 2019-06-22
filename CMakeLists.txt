cmake_minimum_required(VERSION 3.8)

# Point CMake to the custom modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set project information
include(GitVersion)
project(AGIStudio
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
		DESCRIPTION "Editor for Sierra AGI games"
		HOMEPAGE_URL https://deledrius.github.io/agistudio/)
message(STATUS "Project version: " ${PROJECT_VERSION_FULL})

# Create a group for the help files
file(GLOB AGIStudio_HELPFILES help/*.html help/*.css help/*.png)
source_group("Help Files" FILES ${AGIStudio_HELPFILES})
add_custom_target(AGIStudio-Help SOURCES ${AGIStudio_HELPFILES})

# Create the main program
add_subdirectory(src)
