cmake_minimum_required(VERSION 3.0)
PROJECT( kiosktool )
FIND_PACKAGE(KDE4 REQUIRED)

add_subdirectory( icons )
add_subdirectory( src )
add_subdirectory( data )

install( FILES kiosktool.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
