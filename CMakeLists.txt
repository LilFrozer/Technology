cmake_minimum_required(VERSION 4.1)
project(Technology)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(BOOST_INCLUDE_DIR
        NAMES boost/beast.hpp
        PATHS /opt/homebrew/include
        /opt/homebrew/opt/boost/include
        /usr/local/include
        /usr/include
        REQUIRED
)

add_executable(Technology main.cpp
        src/Server.cpp
        src/Connection/Server.h
        src/ClientSession.cpp
        src/Connection/ClientSession.h)

target_include_directories(
        Technology PRIVATE
        ${BOOST_INCLUDE_DIR}
        src
)

# Говорим Beast использовать std::error_code вместо boost::system::error_code
target_compile_definitions(Technology PRIVATE
        BOOST_BEAST_USE_STD_ERIC=1
        BOOST_ASIO_USE_STD_ERIC=1
)
