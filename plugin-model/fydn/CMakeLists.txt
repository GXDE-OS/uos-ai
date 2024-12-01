project(fydn)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Gui REQUIRED)

FILE(GLOB_RECURSE SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
)

set(ASSETS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/uos-ai-assistant/llm/assets/")
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
    Qt5::Gui
    )

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)

if (ENABLE_MODEL_PLUGIN)
    message("install fydn plugin...")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${EXTERNAL_LLM_DIR})

    file(GLOB ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/*")
    install(FILES ${ASSET_FILES} DESTINATION "${ASSETS_INSTALL_DIR}/icons")

    file(GLOB FAQ_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.json")
    install(FILES ${FAQ_FILES} DESTINATION ${ASSETS_INSTALL_DIR})

endif()
