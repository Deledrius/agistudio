project(AGIStudio)
cmake_minimum_required(VERSION 3.8)

# Create a group for the help files
file(GLOB AGIStudio_HELPFILES help/*.html help/*.css)
source_group("Help Files" FILES ${AGIStudio_HELPFILES})
add_custom_target(AGIStudio-Help SOURCES ${AGIStudio_HELPFILES})

# Create the main program
add_subdirectory(src)
