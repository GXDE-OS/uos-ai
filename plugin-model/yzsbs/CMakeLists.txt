project(yzsbs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Network)

FILE(GLOB_RECURSE SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
)

set(ASSETS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/uos-ai-assistant/llm/${PROJECT_NAME}/assets/")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DASSETS_INSTALL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
else()
    add_definitions(-DASSETS_INSTALL_DIR="${ASSETS_INSTALL_DIR}")
endif()

add_library(${PROJECT_NAME}
    SHARED
    ${SRCS}
    )

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    )

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)

include_directories(${CMAKE_SOURCE_DIR}/src)

if (ENABLE_MODEL_YZSBS)
    message("install ztbbb plugin...")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${EXTERNAL_LLM_DIR})

    file(GLOB ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/*")
    install(FILES ${ASSET_FILES} DESTINATION "${ASSETS_INSTALL_DIR}/icons")

    file(GLOB FAQ_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.json")
    install(FILES ${FAQ_FILES} DESTINATION ${ASSETS_INSTALL_DIR})

endif()
