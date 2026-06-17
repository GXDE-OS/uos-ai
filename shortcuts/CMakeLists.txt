# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Shortcut configuration files for dde-keybinding integration
# These configs are installed to /usr/share/dsg/configs/org.deepin.dde.keybinding/
# which is read by dde-service-manager's shortcut plugin.

# Shortcut config files - each subdirectory contains one shortcut definition
file(GLOB SHORTCUT_CONFIGS
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/org.deepin.dde.keybinding.shortcut.app.uos-ai/org.deepin.shortcut.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/org.deepin.dde.keybinding.shortcut.app.uos-ai-followalong/org.deepin.shortcut.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/org.deepin.dde.keybinding.shortcut.app.uos-ai-screenshot/org.deepin.shortcut.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/org.deepin.dde.keybinding.shortcut.app.uos-ai-talk/org.deepin.shortcut.json"
)

# Install shortcut JSON configs to DSG data directory
# Final path: /usr/share/dsg/configs/org.deepin.dde.keybinding/<subdir>/
foreach(CONFIG_FILE ${SHORTCUT_CONFIGS})
    get_filename_component(SUBDIR_NAME ${CONFIG_FILE} DIRECTORY)
    get_filename_component(SUBDIR_BASE ${SUBDIR_NAME} NAME)
    install(FILES ${CONFIG_FILE}
            DESTINATION share/dsg/configs/org.deepin.dde.keybinding/${SUBDIR_BASE})
endforeach()

# Install INI registration file to deepin keybinding directory
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/org.deepin.dde.shortcut.ai.ini
        DESTINATION share/deepin/org.deepin.dde.keybinding)

# Install shortcut translation files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/org.deepin.dde.shortcut.ai.qm
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/org.deepin.dde.shortcut.ai_zh_CN.qm
    DESTINATION share/deepin/org.deepin.dde.keybinding/translations/org.deepin.dde.shortcut.ai)
