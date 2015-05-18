INCLUDE_DIRECTORIES(
        ${CMAKE_SOURCE_DIR}
        ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR} )

set (KDE_DEFAULT_HOME ".kde${_KDE_DEFAULT_HOME_POSTFIX}" CACHE STRING "The default KDE home directory" )
SET( DEFAULT_MIN_UID 500 CACHE PATH "Default minimal UID for filtering the list of users.")
SET( DEFAULT_MIN_GID 500 CACHE PATH "Default minimal GID for filtering the list of groups.")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

set ( kiosktool_SRCS
   component.cpp
   componentPage.cpp
   componentSelectionPage.cpp
   desktopComponent.cpp
   filetypeeditComponent.cpp
   kioskConfigDialog.cpp
   kioskdata.cpp
   ProfileManager.cpp
   uidfiltermodel.cpp
   gidfiltermodel.cpp
   kioskgui.cpp
   kioskrun.cpp
   kiosksync.cpp
   main.cpp
   menueditComponent.cpp
   pageWidget.cpp
   panelComponent.cpp
   profileeditgui.cpp
   profilePropsDialog.cpp
   ProfileSelectorDialog.cpp
   screensaverComponent.cpp
   kconfigraweditor.cpp
)

qt4_add_dbus_interfaces(kiosktool_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.kded.xml)

kde4_add_ui_files ( kiosktool_SRCS
    componentSelectionPage.ui
    componentPage.ui
    kioskConfigDialog.ui
    ProfileSelectorDialog.ui
    profilePropsDialog.ui
    editorDialog.ui
    mainWindow.ui
)

set ( kiosktool_kdedirs_SRCS
   kiosktool-kdedirs.cpp
)

set ( kconfigraweditor_SRCS
    kconfigraweditortest.cpp
)
kde4_add_executable(kconfigraweditortest ${kconfigraweditor_SRCS})
target_link_libraries(kconfigraweditortest ${KDE4_KDECORE_LIBS})


kde4_add_executable(kiosktool ${kiosktool_SRCS})

install(TARGETS kiosktool ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries (kiosktool ${KDE4_KDEUI_LIBS} ${KDE4_KDE3SUPPORT_LIBS} )

kde4_add_executable(kiosktool-kdedirs ${kiosktool_kdedirs_SRCS})
target_link_libraries(kiosktool-kdedirs ${KDE4_KDECORE_LIBS})
install(TARGETS kiosktool-kdedirs ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES kiosktoolui.rc DESTINATION  ${DATA_INSTALL_DIR}/kiosktool )
