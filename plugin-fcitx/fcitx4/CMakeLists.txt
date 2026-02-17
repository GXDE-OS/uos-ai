project(fcitx-uosai)
find_package(Fcitx 4.2.8 REQUIRED)

_fcitx_add_uninstall_target()

pkg_check_modules(DBus "dbus-1" REQUIRED IMPORTED_TARGET)

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories (
    ${PROJECT_BINARY_DIR}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    )

set(fcitx_uosai_sources
    eim.cpp
    config.cpp
    bus.cpp
    )

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

configure_file(fcitx-uosai.desc fcitx-uosai.desc)

fcitx_add_addon_full(uosai DESC
SOURCES ${fcitx_uosai_sources}
IM_CONFIG uosaiplugin.conf
LINK_LIBS PkgConfig::DBus ${LIBINTL_LIBRARIES} X11 Xtst
)

configure_file(config.h.in config.h)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/fcitx)

file(GLOB PLUGIN_FCITX_CONFIGFILE "${CMAKE_BINARY_DIR}/plugin-fcitx/src/fcitx-uosai.conf")
install(FILES ${PLUGIN_FCITX_CONFIGFILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/fcitx/addon/)

file(GLOB PLUGIN_FCITX_DESCFILE "${CMAKE_BINARY_DIR}/plugin-fcitx/src/fcitx-uosai.desc")
install(FILES ${PLUGIN_FCITX_DESCFILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/fcitx/configdesc/)
