set(AGIStudio_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/agistudio.rc

    ${CMAKE_CURRENT_SOURCE_DIR}/appicon.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/app_icon.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left2_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right1.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_close.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_run.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/zoom_minus.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/back.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/leftarrow_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right1_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_logedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_textedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/zoom_minus_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/downarrow_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left1.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/logo.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right2.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_objedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_viewedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/zoom_plus.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/forward.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left1_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right2_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_open.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_wordsedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/zoom_plus_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/home.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/left2.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/right_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/rightarrow_x.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/toolbar_picedit.xpm
    ${CMAKE_CURRENT_SOURCE_DIR}/uparrow_x.xpm
    
    PARENT_SCOPE
)

set(AGIStudio_APPLE_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AGIStudio.icns
)
set_source_files_properties(${AGIStudio_APPLE_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
