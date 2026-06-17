# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# DdeShortcutI18n.cmake
#
# This module provides a helper function to automate the extraction of 
# shortcut display names from JSON configurations into Qt translation files.
#
# The function creates an internal OBJECT library to handle translation generation,
# so you don't need to provide an existing target. Translation files (.qm) are
# generated as independent build artifacts and installed separately.
#
# Usage:
#   find_package(DdeShortcutI18n REQUIRED)
#   dde_shortcut_add_translations(
#       APP_ID "org.deepin.myapp"                          # Required: Application identifier
#       CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configs"   # Optional: Path to JSON configs
#       TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations"  # Optional: Path to .ts files
#       LANGUAGES "zh_CN" "en_US" "fr"                     # Required: List of languages
#   )
#
# Parameters:
#   APP_ID      - (Required) Application identifier, used for naming translation files
#   CONFIG_DIR  - (Optional) Directory containing shortcut JSON configs (default: ./configs)
#   TS_DIR      - (Optional) Directory for .ts translation files (default: ./translations)
#   LANGUAGES   - (Required) List of language codes (e.g., "zh_CN" "en_US")
#
# Generated Targets:
#   - shortcut-i18n-<sanitized-app-id>: Internal OBJECT library for translation
#   - update_shortcut_i18n: Fixed-name target to update shortcut translations
#
# Example:
#   cmake --build build --target update_shortcut_i18n
#
# Note: This function ONLY handles shortcut configuration translations from DConfig JSON files.
#       Application UI translations should be handled separately using standard Qt translation tools.
#       The update_shortcut_i18n target will update ALL applications that use this function.

set(_DDE_I18N_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(dde_shortcut_add_translations)
    set(options)
    set(oneValueArgs APP_ID CONFIG_DIR TS_DIR)
    set(multiValueArgs LANGUAGES)
    cmake_parse_arguments(DDE_I18N "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required parameters
    if(NOT DDE_I18N_APP_ID)
        message(FATAL_ERROR "dde_shortcut_add_translations: APP_ID is required")
    endif()
    if(NOT DDE_I18N_LANGUAGES)
        message(WARNING "dde_shortcut_add_translations: No LANGUAGES specified for ${DDE_I18N_APP_ID}")
        return()
    endif()
    
    # Set defaults
    if(NOT DDE_I18N_CONFIG_DIR)
        set(DDE_I18N_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configs")
    endif()
    if(NOT DDE_I18N_TS_DIR)
        set(DDE_I18N_TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations")
    endif()

    # Ensure the translation directory exists
    if(NOT EXISTS "${DDE_I18N_TS_DIR}")
        file(MAKE_DIRECTORY "${DDE_I18N_TS_DIR}")
    endif()

    # Sanitize APP_ID for use in target names (replace dots with dashes)
    string(REPLACE "." "-" _sanitized_app_id "${DDE_I18N_APP_ID}")
    set(SHORTCUT_I18N_SRC "${CMAKE_CURRENT_BINARY_DIR}/shortcut_i18n_strings_${_sanitized_app_id}.cpp")

    # Find extraction tool
    find_program(EXTRACT_TOOL NAMES extract_shortcuts_i18n)
    if(NOT EXTRACT_TOOL)
        # Fallback for local development if not installed yet
        set(EXTRACT_TOOL "${_DDE_I18N_MODULE_DIR}/../tools/extract_shortcuts_i18n.py")
        if(NOT EXISTS "${EXTRACT_TOOL}")
             message(FATAL_ERROR "dde_shortcut_add_translations: extract_shortcuts_i18n tool not found")
        endif()
    endif()

    # Extract shortcut names from JSON configs
    file(GLOB_RECURSE JSON_CONFIGS "${DDE_I18N_CONFIG_DIR}/*.json")
    add_custom_command(
        OUTPUT ${SHORTCUT_I18N_SRC}
        COMMAND ${EXTRACT_TOOL} 
                ${DDE_I18N_CONFIG_DIR} 
                ${SHORTCUT_I18N_SRC} 
                ${DDE_I18N_APP_ID}
        DEPENDS ${EXTRACT_TOOL} ${JSON_CONFIGS}
        COMMENT "Extracting shortcut names for translation for ${DDE_I18N_APP_ID}"
        VERBATIM
    )

    # Build TS file list
    set(TS_FILES "${DDE_I18N_TS_DIR}/${DDE_I18N_APP_ID}.ts")
    foreach(LANG ${DDE_I18N_LANGUAGES})
        list(APPEND TS_FILES "${DDE_I18N_TS_DIR}/${DDE_I18N_APP_ID}_${LANG}.ts")
    endforeach()

    # Create empty TS files if they don't exist to avoid CMake warnings
    foreach(TS_FILE ${TS_FILES})
        if(NOT EXISTS "${TS_FILE}")
            file(WRITE "${TS_FILE}" "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>\n<TS version=\"2.1\"/>\n")
        endif()
    endforeach()

    # Create internal OBJECT library for translation
    set(_translation_target "shortcut-i18n-${_sanitized_app_id}")
    if(NOT TARGET ${_translation_target})
        add_library(${_translation_target} OBJECT ${SHORTCUT_I18N_SRC})

        # Ensure Qt6::Core is available
        if(NOT TARGET Qt6::Core)
            find_package(Qt6 REQUIRED COMPONENTS Core)
        endif()
        target_link_libraries(${_translation_target} PRIVATE Qt6::Core)

        message(STATUS "Created translation target: ${_translation_target}")
    else()
        target_sources(${_translation_target} PRIVATE ${SHORTCUT_I18N_SRC})
    endif()

    # Generate translations using Qt's built-in commands
    if(COMMAND qt6_add_translations)
        qt6_add_translations(${_translation_target}
            TS_FILES ${TS_FILES}
            SOURCES ${SHORTCUT_I18N_SRC}
            LUPDATE_OPTIONS "-locations" "none"
            LRELEASE_OPTIONS "-silent"
            QM_FILES_OUTPUT_VARIABLE QM_FILES
        )
    elseif(COMMAND qt_add_translations)
        qt_add_translations(${_translation_target}
            TS_FILES ${TS_FILES}
            SOURCES ${SHORTCUT_I18N_SRC}
            LUPDATE_OPTIONS "-locations" "none"
            LRELEASE_OPTIONS "-silent"
            QM_FILES_OUTPUT_VARIABLE QM_FILES
        )
    else()
        message(FATAL_ERROR "dde_shortcut_add_translations: qt6_add_translations or qt_add_translations not found. Please find_package(Qt6 LINGUIST_TOOLS REQUIRED)")
    endif()

    # Create update target (fixed name for all apps)
    find_program(LUPDATE_EXECUTABLE NAMES lupdate lupdate6 PATHS /usr/lib/qt6/bin NO_DEFAULT_PATH)
    if(NOT LUPDATE_EXECUTABLE)
        find_program(LUPDATE_EXECUTABLE NAMES lupdate lupdate6)
    endif()

    if(LUPDATE_EXECUTABLE)
        # Create app-specific internal target
        set(_app_update_target "_update_shortcut_i18n_${_sanitized_app_id}")

        if(NOT TARGET ${_app_update_target})
            add_custom_target(${_app_update_target}
                COMMAND ${LUPDATE_EXECUTABLE} ${SHORTCUT_I18N_SRC} -ts ${TS_FILES} -locations none
                DEPENDS ${SHORTCUT_I18N_SRC}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Updating shortcut i18n for ${DDE_I18N_APP_ID} (DConfig JSON only)"
                VERBATIM
            )
        endif()

        # Create or update the fixed-name public target
        if(NOT TARGET update_shortcut_i18n)
            add_custom_target(update_shortcut_i18n
                DEPENDS ${_app_update_target}
                COMMENT "Update shortcut translations (DConfig JSON only)"
            )
            message(STATUS "Created shortcut i18n update target: update_shortcut_i18n")
        else()
            # Add this app's update target as a dependency
            add_dependencies(update_shortcut_i18n ${_app_update_target})
            message(STATUS "Added ${DDE_I18N_APP_ID} to update_shortcut_i18n target")
        endif()
    endif()

    # Install QM files
    install(FILES ${QM_FILES}
            DESTINATION "share/deepin/org.deepin.dde.keybinding/translations/${DDE_I18N_APP_ID}")
endfunction()
