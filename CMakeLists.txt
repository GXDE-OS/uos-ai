cmake_minimum_required(VERSION 3.10)
project(uos-ai VERSION 0.1 LANGUAGES CXX)
add_compile_definitions(APP_VERSION="${APP_VERSION}")

# 全局配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# generate qm files
if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    execute_process(COMMAND bash "${CMAKE_SOURCE_DIR}/translate_generation.sh" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# 库目录
set(LIB_PLATFORM_DIR "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
add_definitions(-DLIB_PLATFORM_DIR="${LIB_PLATFORM_DIR}")

# 插件
set(EXTERNAL_LLM_DIR "${LIB_PLATFORM_DIR}/uos-ai-assistant/llm")

# 获取系统版本
if (NOT DEFINED BUILD_OS_VERSION OR BUILD_OS_VERSION STREQUAL "")
    # 读取文件并提取Version值
    file(STRINGS "/etc/deepin-version" FILE_CONTENTS REGEX "^Version=")

    # 遍历文件内容，查找Version行
    foreach(LINE ${FILE_CONTENTS})
        # 检查是否是Version行
        if(LINE MATCHES "Version=([0-9]+)")
            # 提取版本号
            set(BUILD_OS_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    message("Get BUILD_OS_VERSION from deepin-version ${BUILD_OS_VERSION}")
else()
    message("catch BUILD_OS_VERSION ${BUILD_OS_VERSION}")
endif()

# Qt
if (${BUILD_OS_VERSION} MATCHES "25")
    find_package(QT NAMES Qt6)
else()
    find_package(QT NAMES Qt5)
endif()

# 配置选项
option(ENABLE_AGENT_PLUGIN "Enable Agent Plugin" ON)
option(ENABLE_MODEL_PLUGINS "Enable Model Plugins" OFF)
option(ENABLE_ASSISTANT "Enable AI Assistant" ON)
option(ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT "Enable Personal Knowledge Assistant" OFF)
option(ENABLE_AI_BAR "Enable AI Bar" OFF)
option(ENABLE_TRAY_PLUGIN "Build Tray Plugin" ON)
option(ENABLE_MCP "Enable MCP Server" ON)

# chatbot
add_definitions(-DENABLE_CHATBOT)

# 模型插件
option(ENABLE_MODEL_FYDN "Enable FYDN Plugin" OFF)
option(ENABLE_MODEL_PPT "Enable PPT Plugin" OFF)
option(ENABLE_MODEL_POSTER "Enable POSTER Plugin" OFF)
option(ENABLE_MODEL_RPA "Enable RPA Plugin" OFF)
option(ENABLE_MODEL_ZTBBD "Enable ZTBBD Plugin" OFF)
option(ENABLE_MODEL_XXZSK "Enable XXZSK Plugin" OFF)
option(ENABLE_MODEL_YZSBS "Enable YZSBS Plugin" OFF)
option(ENABLE_MODEL_TZDL "Enable TZDL Plugin" OFF)

if (NOT ${BUILD_OS_VERSION} MATCHES "25")
    # 增加安全编译参数
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all -fPIC")
    set(CMAKE_EXE_LINKER_FLAGS  "-z relro -z now -z noexecstack -pie")
    message("20 applying security hardening options")
else()
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

# GXDE 的会比较特殊

message("COMPILE ON GXDE ${BUILD_OS_VERSION}")

#set(COMPILE_ON_V25 TRUE)
#add_definitions(-DCOMPILE_ON_V25)

add_definitions(-DCOMPILE_ON_V23)
set(COMPILE_ON_V23 TRUE)

#先默认打开ppt助手
#set(ENABLE_MODEL_PPT ON)
#set(ENABLE_MODEL_POSTER ON)
#set(ENABLE_AI_BAR ON)  # GXDE 不支持 deepin shell

if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|aarch64")
    set(ENABLE_ASSISTANT OFF)
    set(ENABLE_MCP OFF)
#else()
#    set(ENABLE_MODEL_RPA ON)
endif()
message("SYSTEM PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} ${ENABLE_ASSISTANT}")

# Debug Or Release
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DEXTERNAL_LLM_DIR="${CMAKE_BINARY_DIR}/plugin-model")
    set(ENABLE_MODEL_PLUGINS ON)
else()
    add_definitions(-DEXTERNAL_LLM_DIR="${EXTERNAL_LLM_DIR}")
endif()

# Qt Version
if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    message("COMPILE ON QT6")
    add_definitions(-DCOMPILE_ON_QT6)
    set(DTK_VERSION_MAJOR 6)
else()
    message("COMPILE ON QT5")
    set(DTK_VERSION_MAJOR "")
endif()

if (ENABLE_MODEL_FYDN OR ENABLE_MODEL_PPT OR ENABLE_MODEL_POSTER OR ENABLE_MODEL_RPA OR ENABLE_MODEL_ZTBBD OR ENABLE_MODEL_XXZSK OR ENABLE_MODEL_YZSBS OR ENABLE_MODEL_TZDL)
    set(ENABLE_MODEL_PLUGINS ON)
endif()

if (ENABLE_AGENT_PLUGIN)
    add_definitions(-DENABLE_AGENT_PLUGIN)
    install(FILES ${PROJECT_SOURCE_DIR}/assets/agent/.readme DESTINATION share/uos-ai-assistant/agent)
endif()

if (ENABLE_MODEL_PLUGINS)
    add_definitions(-DENABLE_MODEL_PLUGIN)
endif()

if (ENABLE_ASSISTANT)
    add_definitions(-DENABLE_ASSISTANT)
endif()

if (ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT)
    add_definitions(-DENABLE_PERSONAL_KNOWLEDGE_ASSISTANT)
endif()

if (ENABLE_MCP)
    add_definitions(-DENABLE_MCP)
endif()

if (ENABLE_AI_BAR)
    add_subdirectory(plugin-aibar)
    set(ENABLE_TRAY_PLUGIN OFF)
    add_definitions(-DENABLE_AI_BAR)
endif()

if (ENABLE_TRAY_PLUGIN)
    add_subdirectory(plugin-tray)
endif()

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(ocr)
add_subdirectory(plugin-model)
add_subdirectory(plugin-fcitx)
add_subdirectory(plugin-settings)

# 安装帮助文档
install(DIRECTORY ${PROJECT_SOURCE_DIR}/docs/uos-ai-assistant
    DESTINATION share/deepin-manual/manual-assets/application)
