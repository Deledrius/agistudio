find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)

if(MINGW)
    add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WIN6)
endif(MINGW)

if(APPLE)
    set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
endif(APPLE)

set(AGIStudio_HEADERS
    agicommands.h
    game.h
    global.h
    helpwindow.h
    linklist.h
    logedit.h
    logic.h
    menu.h
    midi.h
    object.h
    objedit.h
    options.h
    picedit.h
    picture.h
    preview.h
    resources.h
    roomgen.h
    view.h
    viewedit.h
    words.h
    wordsedit.h
    wutil.h
    bmp2agipic.h
)

set(AGIStudio_RESOURCES
    icon1.ico
    agistudio.rc
    app_icon.xpm
    left.xpm
    left2_x.xpm
    right1.xpm
    toolbar_close.xpm
    toolbar_run.xpm
    zoom_minus.xpm
    back.xpm
    left_x.xpm
    leftarrow_x.xpm
    right1_x.xpm
    toolbar_logedit.xpm
    toolbar_textedit.xpm
    zoom_minus_x.xpm
    downarrow_x.xpm
    left1.xpm
    logo.xpm
    right2.xpm
    toolbar_objedit.xpm
    toolbar_viewedit.xpm
    zoom_plus.xpm
    forward.xpm
    left1_x.xpm
    right.xpm
    right2_x.xpm
    toolbar_open.xpm
    toolbar_wordsedit.xpm
    zoom_plus_x.xpm
    home.xpm
    left2.xpm
    right_x.xpm
    rightarrow_x.xpm
    toolbar_picedit.xpm
    uparrow_x.xpm
)

set(AGIStudio_SOURCES
    agicommands.cpp
    agiplay.cpp
    bpicture.cpp
    game.cpp
    helpwindow.cpp
    linklist.cpp
    logcompile.cpp
    logdecode.cpp
    logedit.cpp
    logic.cpp
    main.cpp
    menu.cpp
    midi.cpp
    object.cpp
    objedit.cpp
    options.cpp
    picedit.cpp
    picture.cpp
    preview.cpp
    resources.cpp
    roomgen.cpp
    view.cpp
    viewedit.cpp
    words.cpp
    wordsedit.cpp
    wutil.cpp
    bmp2agipic.cpp
)

source_group("Header Files" FILES ${AGIStudio_HEADERS})
source_group("Resource Files" FILES ${AGIStudio_RESOURCES})
source_group("Source Files" FILES ${AGIStudio_SOURCES})

add_executable(AGIStudio WIN32 MACOSX_BUNDLE
               ${AGIStudio_HEADERS} ${AGIStudio_SOURCES} ${AGIStudio_RESOURCES})

target_link_libraries(AGIStudio PRIVATE Qt6::Core Qt6::Widgets Qt6::Multimedia)

install(TARGETS AGIStudio
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .)

# Deployment configuration
set(CPACK_COMPONENT_AGISTUDIO_DISPLAY_NAME "AGIStudio")
set(CPACK_COMPONENTS_ALL AGIStudio)

qt_generate_deploy_app_script(
    TARGET AGIStudio
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
