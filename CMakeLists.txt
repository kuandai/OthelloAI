cmake_minimum_required(VERSION 3.16)
project(OthelloBot LANGUAGES C CXX)

# Use Ninja
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCL
find_package(OpenCL REQUIRED)

# rapidyaml and spdlog via FetchContent
include(FetchContent)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# rapidyaml
FetchContent_Declare(
  rapidyaml
  GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rapidyaml)

# GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(googletest)
enable_testing()

# ==== Source Files ====

# Shared engine files (used across multiple targets)
set(ENGINE_SOURCES
  src/othello/board.cpp
  # Add more shared files later (e.g., src/agent/agent.cpp, etc.)
)

# Files specific to othelloplayer binary
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
set(OTHELLOPLAYER_SOURCES ${SOURCES})
list(REMOVE_ITEM OTHELLOPLAYER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/train.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/remote_agent.cpp
)

# ==== Targets ====

# Core engine library
add_library(othello_engine STATIC ${ENGINE_SOURCES})
target_include_directories(othello_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main executable (uses engine)
add_executable(othelloplayer ${OTHELLOPLAYER_SOURCES})
add_dependencies(othelloplayer embed_weights)
target_include_directories(othelloplayer PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_include_directories(othelloplayer PRIVATE ${rapidyaml_SOURCE_DIR}/src)
target_link_libraries(othelloplayer
  PRIVATE
    othello_engine
    OpenCL::OpenCL
    spdlog::spdlog
    ryml
)

# Remote agent
add_executable(remote_agent src/remote_agent.cpp)
target_link_libraries(remote_agent PRIVATE othello_engine)

# Unit tests
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(othello_tests ${TEST_SOURCES})
target_include_directories(othello_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(othello_tests
  PRIVATE
    othello_engine
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(othello_tests)

# Training binary
add_executable(train src/train.cpp)
target_include_directories(train PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${rapidyaml_SOURCE_DIR}/src)
target_link_libraries(train
  PRIVATE
    othello_engine
    OpenCL::OpenCL
    spdlog::spdlog
    ryml
)

# Embed tool
add_executable(embed_model tools/embed_model.cpp)

# Embed model command
set(MODEL_INPUT ${CMAKE_SOURCE_DIR}/weights/weights.bin)
set(MODEL_HEADER ${CMAKE_BINARY_DIR}/generated/weights.hpp)

add_custom_command(
  OUTPUT ${MODEL_HEADER}
  COMMAND embed_model ${MODEL_INPUT} ${MODEL_HEADER}
  DEPENDS embed_model ${MODEL_INPUT}
  COMMENT "Embedding weights into header"
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
add_custom_target(embed_weights DEPENDS ${MODEL_HEADER})