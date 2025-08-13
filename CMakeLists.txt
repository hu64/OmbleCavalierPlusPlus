cmake_minimum_required(VERSION 3.10)
project(OmbleCavalier++ CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include folder
include_directories(${PROJECT_SOURCE_DIR}/include)

# Executable
add_executable(omble_cavalier++ src/main.cpp)