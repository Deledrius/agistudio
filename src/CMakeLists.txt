find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)

if(MINGW)
    add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WIN6)
endif(MINGW)

set(AGIStudio_HEADERS
    agicommands.h
    game.h
    helpwindow.h
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

set(AGIStudio_UI
    ui/mainmenu.ui
    ui/objedit.ui
    ui/options.ui
    ui/picedit.ui
    ui/resources.ui
    ui/textedit.ui
    ui/viewedit.ui
    ui/wordsedit.ui
    ui/wordsfind.ui
)

set(AGIStudio_UIRESOURCES
    ui/agistudio.qrc
)
qt_add_resources(RCC_SOURCES ${AGIStudio_UIRESOURCES})

# Create a group for the template project files
set(AGIStudio_TEMPLATEFILES 
	../template/logdir ../template/object ../template/picdir ../template/snddir ../template/viewdir ../template/vol.0 ../template/words.tok )
file(GLOB AGIStudio_TEMPLATESRCFILES ../template/src/*.txt)
source_group("Template Files" FILES ${AGIStudio_TEMPLATEFILES} ${AGIStudio_TEMPLATESRCFILES})

# Create a group for the help files
file(GLOB AGIStudio_HELPFILES ../help/*.html ../help/*.css ../help/*.png)
source_group("Help Files" FILES ${AGIStudio_HELPFILES})

if (APPLE)
    set_source_files_properties(${AGIStudio_TEMPLATEFILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/template")
    set_source_files_properties(${AGIStudio_TEMPLATESRCFILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/template/src")
    set_source_files_properties(${AGIStudio_HELPFILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/help")
else ()
    install(FILES ${AGIStudio_TEMPLATEFILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/template)
    install(FILES ${AGIStudio_TEMPLATESRCFILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/template/src)
    install(FILES ${AGIStudio_HELPFILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/help)
endif()

# Create groups for the AGIStudio project files
source_group("Header Files" FILES ${AGIStudio_HEADERS})
source_group("Resource Files" FILES ${AGIStudio_RESOURCES} ${AGIStudio_UIRESOURCES})
source_group("Source Files" FILES ${AGIStudio_SOURCES})
source_group("UI Files" FILES ${AGIStudio_UI})

add_executable(AGIStudio WIN32 MACOSX_BUNDLE
               ${AGIStudio_HEADERS} ${AGIStudio_SOURCES} ${AGIStudio_RESOURCES} ${AGIStudio_UI} ${RCC_SOURCES}
               ${AGIStudio_TEMPLATEFILES} ${AGIStudio_TEMPLATESRCFILES} ${AGIStudio_HELPFILES})

target_link_libraries(AGIStudio PRIVATE Qt6::Core Qt6::Widgets Qt6::Multimedia)

install(TARGETS AGIStudio
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .)

#add_dependencies(AGIStudio AGIStudio-Help)

# Deployment configuration
set(CPACK_COMPONENT_AGISTUDIO_DISPLAY_NAME "AGIStudio")
set(CPACK_COMPONENTS_ALL AGIStudio)

qt_generate_deploy_app_script(
    TARGET AGIStudio
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
