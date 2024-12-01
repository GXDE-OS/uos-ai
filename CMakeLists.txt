cmake_minimum_required(VERSION 3.10)

project(uos-ai VERSION 0.1 LANGUAGES CXX)

add_compile_definitions(APP_VERSION="${APP_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# 增加安全编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all -fPIC")
set(CMAKE_EXE_LINKER_FLAGS  "-z relro -z now -z noexecstack -pie")

# generate qm files
if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    execute_process(COMMAND bash "${CMAKE_SOURCE_DIR}/translate_generation.sh" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

set(LIB_PLATFORM_DIR "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
add_definitions(-DLIB_PLATFORM_DIR="${LIB_PLATFORM_DIR}")

set(EXTERNAL_LLM_DIR "${LIB_PLATFORM_DIR}/uos-ai-assistant/llm")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DEXTERNAL_LLM_DIR="${CMAKE_BINARY_DIR}/plugin-model")
else()
    add_definitions(-DEXTERNAL_LLM_DIR="${EXTERNAL_LLM_DIR}")
endif()

include_directories("${CMAKE_SOURCE_DIR}/include")

find_package(DtkCMake REQUIRED)

#Indentify the version
if (1 OR ${DEEPIN_OS_VERSION} MATCHES "23")
    add_definitions(-DCOMPILE_ON_V23)
    set(COMPILE_ON_V23 TRUE)
    message("COMPILE ON v23")
else()
    add_definitions(-DCOMPILE_ON_V20)
    set(COMPILE_ON_V20 TRUE)
    message("COMPILE ON v20")

    set(ENABLE_MODEL_PLUGIN TRUE)
    add_definitions(-DENABLE_MODEL_PLUGIN)
endif()

add_subdirectory(plugin-model)

#add_definitions(-DDEBUG_LOG)

# Find the library
find_package(DtkCore REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5WebSockets REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(KF5GlobalAccel COMPONENTS KF5GlobalAccel REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)
pkg_check_modules(QGsettings REQUIRED gsettings-qt)
pkg_check_modules(AtSpi REQUIRED atspi-2)
pkg_check_modules(Gobject REQUIRED gobject-2.0)

#-------------------------EXE------------------------
SET(EXENAME uos-ai-assistant)

#遍历所有Cpp文件
file(GLOB_RECURSE EXE_FILES src/*.h src/*.cpp src/*.qrc)
foreach(filepath ${EXE_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND RELATIVE_EXE_FILES ${filename})
endforeach(filepath)

add_executable(${EXENAME}
    ${RELATIVE_EXE_FILES}
)

target_include_directories(${EXENAME} PUBLIC
    src
    src/dbs
    src/dbs/dbwrapper
    src/dbs/tables
    src/tables
    src/dbus
    src/gui/mgmt
    src/gui/chat
    src/gui/screen
    src/http
    src/llm
    src/llm/360
    src/llm/zhipu
    src/llm/openai
    src/llm/iflytek
    src/llm/baidu
    src/llm/localtext2image
    src/llm/common
    src/socket
    src/wrapper
    src/tas
    src/utils
    src/audio
    src/audio/server
    src/audio/trans
    src/oscontrol
    src/oscontrol/ability
    src/tools
    ${DFrameworkDBus_INCLUDE_DIRS}
    ${QGsettings_INCLUDE_DIRS}
    ${AtSpi_INCLUDE_DIRS}
    ${Gobject_INCLUDE_DIRS}
)

target_link_libraries(${EXENAME}
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    ${Qt5Sql_LIBRARIES}
    ${Qt5WebSockets_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    ${Qt5Multimedia_LIBRARIES}
    ${DFrameworkDBus_LIBRARIES}
    ${QGsettings_LIBRARIES}
    ${AtSpi_LIBRARIES}
    ${Gobject_LIBRARIES}
    Qt5::WebEngineWidgets
    KF5::GlobalAccel
    crypto
)

# Install settings
set(CMAKE_INSTALL_PREFIX /usr)

# Install bin
install(TARGETS ${EXENAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Install qm files
file(GLOB EXE_QM_FILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-assistant*.qm")
install(FILES ${EXE_QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${EXENAME}/translations/)

# Install service
file(GLOB SERVICE_FILES "assets/*.service")
install(FILES ${SERVICE_FILES} DESTINATION share/dbus-1/services)

# Install credits
file(GLOB CREDITS_FILES "assets/${EXENAME}.json")
install(FILES ${CREDITS_FILES} DESTINATION share/deepin/credits)

# Install functions
set(FUNCTIONS_DESTINATION "/usr/lib/${EXENAME}/functions")
add_definitions(-DFUNCTIONPATH="${FUNCTIONS_DESTINATION}")
file(GLOB FUNCTIONS_FILES "assets/functions/*")
install(FILES ${FUNCTIONS_FILES} DESTINATION ${FUNCTIONS_DESTINATION})

# Install audio
set(AUDIO_DESTINATION "/usr/lib/${EXENAME}/audio")
add_definitions(-DAUDIOPATH="${AUDIO_DESTINATION}")
file(GLOB AUDIO_FILES "assets/audio/*")
install(FILES ${AUDIO_FILES} DESTINATION ${AUDIO_DESTINATION})

# install desktop
message(${PROJECT_SOURCE_DIR})
install(FILES ${PROJECT_SOURCE_DIR}/assets/app/uos-ai-assistant.desktop
    DESTINATION share/applications)

#-------------------------plugin-tray------------------------
SET(TRAYPLUGINNAME uos-ai-tray)

#遍历所有文件
file(GLOB_RECURSE PLUGIN_TRAY_FILES plugin-tray/*.h plugin-tray/*.cpp plugin-tray/*.qrc)

foreach(filepath ${PLUGIN_TRAY_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND RELATIVE_PLUGIN_TRAY_FILES ${filename})
endforeach(filepath)

add_library(${TRAYPLUGINNAME} MODULE
    ${RELATIVE_PLUGIN_TRAY_FILES}
)

target_include_directories(${TRAYPLUGINNAME} PUBLIC
    plugin-tray
    ${Qt5Widgets_INCLUDE_DIRS}
    ${DtkWidget_INCLUDE_DIRS}
    /usr/include/dde-dock   #${DdeDockInterface_INCLUDE_DIRS} 1030 not support
)

target_link_libraries(${TRAYPLUGINNAME} PRIVATE
    ${DtkWidget_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
)

# Install bin
install(TARGETS ${TRAYPLUGINNAME} LIBRARY DESTINATION lib/dde-dock/plugins)

# Install qm files
file(GLOB PLUGIN_TRAY_QMFILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-tray*.qm")
install(FILES ${PLUGIN_TRAY_QMFILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${TRAYPLUGINNAME}/translations/)

# Install icon
install(FILES ${CMAKE_SOURCE_DIR}/plugin-tray/assets/icons/deepin/builtin/uosai.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dde-dock/icons/dcc-setting/)
install(FILES ${CMAKE_SOURCE_DIR}/assets/images/UosAiAssistant.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

#-------------------------plugin-settings------------------------
SET(SETTINGSPLUGINNAME uos-ai-settings)

#遍历所有文件
file(GLOB_RECURSE PLUGIN_SETTINGS_FILES plugin-settings/*.h plugin-settings/*.cpp plugin-settings/*.qrc)

# Install qm files
file(GLOB PLUGIN_SETTINGS_QMFILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-settings*.qm")
install(FILES ${PLUGIN_SETTINGS_QMFILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${SETTINGSPLUGINNAME}/translations/)
