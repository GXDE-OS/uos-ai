project(uos-ai-settings)

#遍历所有文件
file(GLOB_RECURSE PLUGIN_SETTINGS_FILES plugin-settings/*.h plugin-settings/*.cpp plugin-settings/*.qrc)

# Install qm files
file(GLOB PLUGIN_SETTINGS_QMFILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-settings*.qm")
install(FILES ${PLUGIN_SETTINGS_QMFILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations/)
