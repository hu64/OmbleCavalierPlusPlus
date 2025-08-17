cmake_minimum_required(VERSION 3.10)
project(OmbleCavalier++ CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # optional, avoids compiler-specific extensions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

# Add include folder
include_directories(${PROJECT_SOURCE_DIR}/include)

# Executable
add_executable(omble_cavalier++ src/main.cpp)