
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

add_library(aibar SHARED ${SRCS})

target_link_libraries(aibar PRIVATE
    Dde::Shell
)

ds_install_package(PACKAGE org.deepin.ds.dock.aibar TARGET aibar)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.aibar)

include(${CMAKE_SOURCE_DIR}/install_dconfig.cmake)
INSTALL_DCONFIG("uos-ai-assistant.aibar.json")
