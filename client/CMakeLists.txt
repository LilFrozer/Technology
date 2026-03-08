cmake_minimum_required(VERSION 3.19)
project(client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

find_path(BOOST_INCLUDE_DIR
    NAMES boost/beast.hpp
    PATHS /opt/homebrew/include
    /opt/homebrew/opt/boost/include
    /usr/local/include
    /usr/include
    REQUIRED
)

qt_add_executable(client
    WIN32 MACOSX_BUNDLE
    main.cpp
    client.cpp
    client.h
    client.ui
)

target_link_libraries(client
    PRIVATE
        ${BOOST_INCLUDE_DIR}
        Qt::Core
        Qt::Widgets
)

target_compile_definitions(client PRIVATE
    BOOST_BEAST_USE_STD_ERIC=1
    BOOST_ASIO_USE_STD_ERIC=1
)


include(GNUInstallDirs)

install(TARGETS client
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET client
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
