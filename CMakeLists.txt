cmake_minimum_required(VERSION 3.20)
project(FinalProjectCPP CXX)

set(CMAKE_CXX_STANDARD 23)
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

# ---- Test target ----
set(MODEL_SOURCES
    model/Commit.cpp
    model/StandardCommit.cpp
    model/TrackedFile.cpp
    model/Validator.cpp
    model/DiffEngine.cpp
)

# View sources exercised by the test suite (pure formatting helpers live here).
set(VIEW_SOURCES
    view/ConsoleView.cpp
)

add_executable(TestRunner
    tests/test_main.cpp
    ${MODEL_SOURCES}
    ${VIEW_SOURCES}
)

set_target_properties(TestRunner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
)

target_include_directories(TestRunner PRIVATE model view)

add_custom_target(RunTests
    COMMAND ${CMAKE_SOURCE_DIR}/tests/TestRunner > ${CMAKE_SOURCE_DIR}/tests/test_output.txt 2>&1
    DEPENDS TestRunner
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running tests — output written to tests/test_output.txt"
)
