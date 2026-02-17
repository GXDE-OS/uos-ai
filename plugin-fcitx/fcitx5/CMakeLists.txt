project(uosai)

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_CXX_STANDARD 17)

find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

add_definitions(${DBUS_CFLAGS_OTHERS})

include_directories(/usr/include/Fcitx5/Core)
include_directories(/usr/include/Fcitx5/Utils)
include_directories(/usr/include/Fcitx5/Module)

add_library(${PROJECT_NAME} MODULE uosai.cpp)

target_link_libraries(${PROJECT_NAME}
    Fcitx5::Core
    Fcitx5::Utils
    ${DBUS_LIBRARIES}
)

# if (ENABLE_X11)
#     target_link_libraries(fcitx4frontend Fcitx5::Module::XCB XCB::XCB)
# endif()


configure_file(uosai.conf.in.in uosai.conf.in @ONLY)

fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/uosai.conf.in uosai.conf)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/uosai.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)

install(TARGETS uosai DESTINATION "${FCITX_INSTALL_ADDONDIR}")

