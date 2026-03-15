project(uos-ai-tray)

# 因为 GXDE DOCK 使用 Qt5 构建，所以该部分需使用 Qt5 构建
find_package(QT NAMES Qt5)
message("COMPILE ON QT5")
set(DTK_VERSION_MAJOR "")
set(QT_VERSION_MAJOR "5")

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Widgets Svg DBus)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget )

#遍历所有文件
file(GLOB_RECURSE PLUGIN_TRAY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)

add_library(${PROJECT_NAME} MODULE
  ${PLUGIN_TRAY_FILES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
  /usr/include/gxde-dock   #${DdeDockInterface_INCLUDE_DIRS} 1030 not support
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Svg
  Qt${QT_VERSION_MAJOR}::DBus
  Dtk${DTK_VERSION_MAJOR}::Widget
)

# Install bin
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/gxde-dock/plugins)

# Install qm files
file(GLOB PLUGIN_TRAY_QMFILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-tray*.qm")
install(FILES ${PLUGIN_TRAY_QMFILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations/)

# Install icon
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/deepin/builtin/uosai.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gxde-dock/icons/dcc-setting/)
