cmake_minimum_required(VERSION 3.14)

# Point CMake to the custom modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set project information
include(GitVersion)
project(AGIStudio
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
		DESCRIPTION "Editor for Sierra AGI games"
		HOMEPAGE_URL https://deledrius.github.io/agistudio/)
message(STATUS "Project version: " ${PROJECT_VERSION_FULL})

set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

# Create a group for the help files
file(GLOB AGIStudio_HELPFILES help/*.html help/*.css help/*.png)
source_group("Help Files" FILES ${AGIStudio_HELPFILES})
add_custom_target(AGIStudio-Help SOURCES ${AGIStudio_HELPFILES})
install(FILES ${AGIStudio_HELPFILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/help)

# Create the main program
add_subdirectory(src)

# Set up package information
set(CPACK_PACKAGE_NAME "AGIStudio")
set(CPACK_PACKAGE_VENDOR "AGIStudio Developers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AGIStudio - Sierra AGI Game Editor")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_FULL})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AGIStudio")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "AGIStudio;AGI Studio")

include(CPack)
