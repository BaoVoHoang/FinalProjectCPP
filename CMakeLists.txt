<<<<<<< HEAD
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
=======
cmake_minimum_required(VERSION 4.2)
project(FinalProjectCPP)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(model)

add_executable(FinalProjectCPP
        model/Commit.cpp
        model/Commit.h
        model/DiffEngine.cpp
        model/DiffEngine.h
        model/StandardCommit.cpp
        model/StandardCommit.h
        model/TrackedFile.cpp
        model/TrackedFile.h
        model/Validator.cpp
        model/Validator.h)
>>>>>>> 5f9a512e6e6355ff4c2e0d21cb4bc582c61de20f
