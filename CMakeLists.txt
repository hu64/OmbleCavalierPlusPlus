cmake_minimum_required(VERSION 3.10)
project(OmbleCavalier++ CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # optional, avoids compiler-specific extensions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native")

# Enable CTest
enable_testing()
include(CTest)

# Add include folder”
include_directories(${PROJECT_SOURCE_DIR}/include)

# Executable
add_executable(omble_cavalier++ src/main.cpp)

# Create individual tests for each puzzle
add_test(NAME "Mate_in_2_a2a6" COMMAND omble_cavalier++ --test "kbK5/pp6/1P6/8/8/8/R7/8 w - - 0 2" "a2a6" 4)
add_test(NAME "Black_wins_queen_c8g4" COMMAND omble_cavalier++ --test "rnbqkbnr/ppp2ppp/3p4/4p3/4P1Q1/8/PPPP1PPP/RNB1KBNR b KQkq - 1 3" "c8g4" 6)
add_test(NAME "White_wins_queen_g5d8" COMMAND omble_cavalier++ --test "rnbqkbnr/1pp2ppp/p2p4/4p1B1/4P3/3P4/PPP2PPP/RN1QKBNR w KQkq - 0 4" "g5d8" 6)
add_test(NAME "Bishop_to_c5" COMMAND omble_cavalier++ --test "r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1" "f8c5" 6)
add_test(NAME "Queen_to_e3" COMMAND omble_cavalier++ --test "1r5k/5ppp/3Q4/8/8/Prq3P1/2P1K2P/3R1R2 b - - 5 27" "c3e3" 6)
add_test(NAME "Mate_in_2_d6c7" COMMAND omble_cavalier++ --test "8/1Q6/2PBK3/k7/8/2P2P2/8/7q w - - 7 63" "d6c7" 6)
add_test(NAME "Mate_in_3_c4b5" COMMAND omble_cavalier++ --test "r3k2r/ppp2Npp/1b5n/4p2b/2B1P2q/BQP2P2/P5PP/RN5K w kq - 1 0" "c4b5" 6)
add_test(NAME "Mate_in_3_f5f2" COMMAND omble_cavalier++ --test "r2n1rk1/1ppb2pp/1p1p4/3Ppq1n/2B3P1/2P4P/PP1N1P1K/R2Q1RN1 b - - 0 1" "f5f2" 6)
add_test(NAME "Mate_in_6_b4a5" COMMAND omble_cavalier++ --test "8/8/8/3k4/1Q1Np2p/1p2P2P/1Pp2b2/2K5 w - - 1 50" "b4a5" 12)

# Run all tests at once
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS omble_cavalier++
)