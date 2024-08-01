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
if(WIN32)
	set(CMAKE_INSTALL_BINDIR .)
else()
	include(GNUInstallDirs)
endif()

# Create the main program
add_subdirectory(src)

# Set up package information
set(CPACK_PACKAGE_NAME "AGIStudio")
set(CPACK_PACKAGE_VENDOR "AGIStudio Developers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AGIStudio - Sierra AGI Game Editor")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_FULL})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AGIStudio")
set(CPACK_PACKAGE_EXECUTABLES "AGIStudio;AGI Studio")

set(CPACK_NSIS_EXECUTABLES_DIRECTORY ${CMAKE_INSTALL_BINDIR})
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)

include(CPack)
