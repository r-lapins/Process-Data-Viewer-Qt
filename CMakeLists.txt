cmake_minimum_required(VERSION 3.21)

project(process-data-viewer-qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

qt_standard_project_setup()

# core library
set(PDT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PDT_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(PDT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/process-data-toolkit)

# application
add_subdirectory(app)
