
project(aibar)

FILE(GLOB_RECURSE SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc"
)

find_package(Qt6 COMPONENTS Core Gui Quick DBus REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
find_package(Dtk6 COMPONENTS Core Gui REQUIRED)
find_package(DDEShell REQUIRED)
find_package(DDEShellDock REQUIRED)

# xdgactivation.h is available since DDEShell 2.0.44
if(DDEShell_VERSION VERSION_GREATER_EQUAL "2.0.44")
    message("DDEShell version is >= 2.0.44, have xdgactivation.h")
    set(HAVE_DDE_SHELL_XDG_ACTIVATION TRUE)
else()
    message("DDEShell version is < 2.0.44, no xdgactivation.h")
endif()


add_library(aibar SHARED ${SRCS})

if (COMPILE_ON_V25 AND HAVE_DDE_SHELL_XDG_ACTIVATION)
    target_compile_definitions(aibar PRIVATE HAVE_DDE_SHELL_XDG_ACTIVATION)
endif()

target_link_libraries(aibar PRIVATE
    Dde::Shell
    Dde::ShellDock
)

ds_install_package(PACKAGE org.deepin.ds.dock.aibar TARGET aibar)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.aibar)

include(${CMAKE_SOURCE_DIR}/install_dconfig.cmake)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/builtin/icons/dcc-uos-ai.dci
        DESTINATION /usr/share/dde-dock/icons/dcc-setting/)
INSTALL_DCONFIG("uos-ai-assistant.aibar.json")

