cmake_minimum_required(VERSION 3.20)
project(FinalProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/target)

file(GLOB_RECURSE SOURCES
    main.cpp
    model/*.cpp
    view/*.cpp
    controller/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    model
    view
    controller
)
