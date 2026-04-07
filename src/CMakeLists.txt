project(uos-ai-assistant)

#遍历所有文件
file(GLOB_RECURSE EXE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)

# 动态查找 Qt 包
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    DBus
    Concurrent
    Sql
    WebSockets
    Svg
    Multimedia
)

# Find the library
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Core Widget )
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED IMPORTED_TARGET poppler-qt6)

add_definitions(-DENABLE_FREEACCOUNT)

pkg_check_modules(AtSpi REQUIRED atspi-2)
pkg_check_modules(Gobject REQUIRED gobject-2.0)
pkg_check_modules(Docparser REQUIRED IMPORTED_TARGET docparser)
pkg_check_modules(XTST REQUIRED xtst)
pkg_check_modules(JWT REQUIRED libjwt)
pkg_check_modules(GIO_LIB REQUIRED
        gio-unix-2.0
        glib-2.0
)

# yaml-cpp for parsing YAML front matter
find_package(yaml-cpp REQUIRED)

find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/chatbot/proto/pbbp2.proto
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})  # generated pbbp2.pb.h

if (COMPILE_ON_V25)
    pkg_check_modules(WL_PROTOCOLS REQUIRED IMPORTED_TARGET wayland-protocols)
    pkg_get_variable(WL_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
    add_executable(${PROJECT_NAME}
        ${EXE_FILES}
        ${PROTO_SRCS}
        ../treeland/ddeshellwayland.h
        ../treeland/ddeshellwayland.cpp
        ../treeland/datacontrolmanagerv1.h
        ../treeland/datacontrolmanagerv1.cpp
        ../treeland/treelandclipboard.h
        ../treeland/treelandclipboard.cpp
        ../treeland/treelandddeactivev1.h
        ../treeland/treelandddeactivev1.cpp
        ../treeland/treelandeventmonitor.h
        ../treeland/treelandeventmonitor.cpp
        ../layershell/dlayershellwindow.h
        ../layershell/dlayershellwindow.cpp
        ../layershell/qwaylandlayershellsurface_p.h
        ../layershell/qwaylandlayershellsurface.cpp
        ../layershell/qwaylandlayershellintegration_p.h
        ../layershell/qwaylandlayershellintegration.cpp
    )

    qt_generate_wayland_protocol_client_sources(${PROJECT_NAME}
        NO_INCLUDE_CORE_ONLY
        FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-dde-shell-v1.xml
        ${WLR_PROTOCOLS_DATADIR}/unstable/wlr-data-control-unstable-v1.xml
        ${WL_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml
        ${CMAKE_SOURCE_DIR}/layershell/protocol/wlr-layer-shell-unstable-v1.xml
    )
elseif(COMPILE_ON_V20)
    add_executable(${PROJECT_NAME}
        ${EXE_FILES}
        ${PROTO_SRCS}
        ../wayland/waylandclipboard.h
        ../wayland/waylandclipboard.cpp
        ../wayland/waylandeventmonitor.h
        ../wayland/waylandeventmonitor.cpp
    )
else()
    add_executable(${PROJECT_NAME}
        ${EXE_FILES}
        ${PROTO_SRCS}
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/agent
    ${CMAKE_CURRENT_SOURCE_DIR}/dbs
    ${CMAKE_CURRENT_SOURCE_DIR}/dbs/dbwrapper
    ${CMAKE_CURRENT_SOURCE_DIR}/dbs/tables
    ${CMAKE_CURRENT_SOURCE_DIR}/tables
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/mgmt
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/chat
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/screen
    ${CMAKE_CURRENT_SOURCE_DIR}/http
    ${CMAKE_CURRENT_SOURCE_DIR}/llm
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/360
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/zhipu
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/openai
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/iflytek
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/baidu
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/localtext2image
    ${CMAKE_CURRENT_SOURCE_DIR}/llm/common
    ${CMAKE_CURRENT_SOURCE_DIR}/socket
    ${CMAKE_CURRENT_SOURCE_DIR}/wrapper
    ${CMAKE_CURRENT_SOURCE_DIR}/tas
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/server
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/trans
    ${CMAKE_CURRENT_SOURCE_DIR}/oscontrol
    ${CMAKE_CURRENT_SOURCE_DIR}/oscontrol/ability
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/wordwizard
    ${CMAKE_CURRENT_SOURCE_DIR}/audiowizard
    ${CMAKE_CURRENT_SOURCE_DIR}/chatbot
    ${AtSpi_INCLUDE_DIRS}
    ${Gobject_INCLUDE_DIRS}
    ${GIO_LIB_INCLUDE_DIRS}
    ${POPPLER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/3rdparty/qmarkdowntextedit
)

if (COMPILE_ON_SW_20)
    pkg_check_modules(QTWEB REQUIRED Qt5WebEngineWidgets)
    target_include_directories(${PROJECT_NAME} PUBLIC ${QTWEB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${QTWEB_LIBRARIES})
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngineWidgets)
    target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
endif()

# Include agent directory
include(${CMAKE_CURRENT_SOURCE_DIR}/agent/CMakeLists.txt)

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Multimedia

    Dtk${DTK_VERSION_MAJOR}::Core
    Dtk${DTK_VERSION_MAJOR}::Widget

    ${AtSpi_LIBRARIES}
    ${Gobject_LIBRARIES}
    ${X11_LIBRARIES}
    ${XTST_LIBRARIES}
    crypto
    PkgConfig::Docparser
    ${GIO_LIB_LIBRARIES}
    qmarkdowntextedit
    ${JWT_LIBRARIES}
    PkgConfig::POPPLER
    yaml-cpp
    pthread
    ${Protobuf_LIBRARIES}
)

# Install bin
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Install qm files
file(GLOB EXE_QM_FILES "${CMAKE_SOURCE_DIR}/translations/uos-ai-assistant*.qm")
install(FILES ${EXE_QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations/)

# Install service
file(GLOB SERVICE_FILES "${CMAKE_SOURCE_DIR}/assets/*.service")
install(FILES ${SERVICE_FILES} DESTINATION share/dbus-1/services)

# Install credits
file(GLOB CREDITS_FILES "${CMAKE_SOURCE_DIR}/assets/${PROJECT_NAME}.json")
install(FILES ${CREDITS_FILES} DESTINATION share/deepin/credits)

# Install functions
set(FUNCTIONS_DESTINATION "/usr/lib/${PROJECT_NAME}/functions")
add_definitions(-DFUNCTIONPATH="${FUNCTIONS_DESTINATION}")
file(GLOB FUNCTIONS_FILES "${CMAKE_SOURCE_DIR}/assets/functions/*")
install(FILES ${FUNCTIONS_FILES} DESTINATION ${FUNCTIONS_DESTINATION})

# Install audio
set(AUDIO_DESTINATION "/usr/lib/${PROJECT_NAME}/audio")
add_definitions(-DAUDIOPATH="${AUDIO_DESTINATION}")
file(GLOB AUDIO_FILES "${CMAKE_SOURCE_DIR}/assets/audio/*")
install(FILES ${AUDIO_FILES} DESTINATION ${AUDIO_DESTINATION})

# Install default skills
set(SKILLS_DEFAULT_DESTINATION "/usr/lib/${PROJECT_NAME}/skills-default")
add_definitions(-DUOS_AI_SKILLS_DEFAULT="${SKILLS_DEFAULT_DESTINATION}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/skills-default/"
    DESTINATION ${SKILLS_DEFAULT_DESTINATION}
    PATTERN ".git*" EXCLUDE)

# install desktop
# Configure desktop file based on OS version (different icons for v20 and v25)
if (COMPILE_ON_V25)
    set(DESKTOP_ICON "UosAiAssistant")
else()
    set(DESKTOP_ICON "UosAiAssistantLogo")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/assets/app/uos-ai-assistant.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/uos-ai-assistant.desktop
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/uos-ai-assistant.desktop
    DESTINATION share/applications)

# install context menu configuration
if (ENABLE_ASSISTANT)
install(FILES ${CMAKE_SOURCE_DIR}/assets/configs/uos-ai-knowledgebase.conf
    DESTINATION share/applications/context-menus)
endif()

# xdg-autostart
install(FILES ${CMAKE_SOURCE_DIR}/assets/autostart/uos-ai-assistant.autostart.desktop
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)

# install tts.sh shortcut
file(GLOB TTS_SH "${CMAKE_SOURCE_DIR}/assets/shell/*.sh")
install(FILES ${TTS_SH}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/shell/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/assets/images/UosAiAssistantLogo.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

include(${CMAKE_SOURCE_DIR}/install_dconfig.cmake)
INSTALL_DCONFIG("uos-ai-assistant.audiowizard.json")
INSTALL_DCONFIG("uos-ai-assistant.tts.json")
INSTALL_DCONFIG("uos-ai-assistant.wordwizard.json")
INSTALL_DCONFIG("uos-ai-assistant.aiquick.json")
INSTALL_DCONFIG("uos-ai-assistant.llm.json")
INSTALL_DCONFIG("uos-ai-assistant.mcp.json")
