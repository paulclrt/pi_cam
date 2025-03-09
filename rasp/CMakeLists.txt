cmake_minimum_required(VERSION 3.10)

project(pi_cam VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/crow")

find_package(Crow REQUIRED)

add_executable(server src/main.cpp)



target_link_libraries(
    server
    PUBLIC Crow::Crow
)
