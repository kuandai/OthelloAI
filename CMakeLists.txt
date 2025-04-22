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

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1 # or the version your spdlog expects
)
FetchContent_MakeAvailable(fmt)

# spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
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

# ==== Embed Tools & Generated Files ====

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# Build the code generator tools first
add_executable(embed_model tools/embed_model.cpp)
add_executable(embed_kernels tools/embed_kernels.cpp)

# Setup paths for generated content
set(MODEL_INPUT ${CMAKE_SOURCE_DIR}/weights/weights.bin)
set(MODEL_HEADER ${CMAKE_BINARY_DIR}/generated/weights.hpp)
set(KERNELS_INPUT_DIR ${CMAKE_SOURCE_DIR}/kernels)
set(KERNELS_HEADER ${CMAKE_BINARY_DIR}/generated/kernels.hpp)

# Create custom commands for generating the header files
add_custom_command(
  OUTPUT ${MODEL_HEADER}
  COMMAND ${CMAKE_BINARY_DIR}/embed_model ${MODEL_INPUT} ${MODEL_HEADER}
  DEPENDS embed_model ${MODEL_INPUT}
  COMMENT "Embedding weights into header"
  VERBATIM
)

add_custom_command(
  OUTPUT ${KERNELS_HEADER}
  COMMAND ${CMAKE_BINARY_DIR}/embed_kernels ${KERNELS_INPUT_DIR} ${KERNELS_HEADER}
  DEPENDS embed_kernels ${KERNELS_INPUT_DIR}
  COMMENT "Embedding OpenCL kernels into header"
  VERBATIM
)

# Create a library for the generated files to force their creation
add_library(generated_headers INTERFACE)
target_include_directories(generated_headers INTERFACE ${CMAKE_BINARY_DIR})
add_custom_target(
  generate_headers
  DEPENDS ${MODEL_HEADER} ${KERNELS_HEADER}
)
add_dependencies(generated_headers generate_headers)

# ==== Source Files ====

# Shared engine files (used across multiple targets)
set(ENGINE_SOURCES
  src/othello/board.cpp
  src/opencl/context.cpp
  src/model/inference_model.cpp
  # Add more shared files later
)

# ==== Targets ====

# Core engine library
add_library(othello_engine STATIC ${ENGINE_SOURCES})
target_include_directories(othello_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(othello_engine PUBLIC ${CMAKE_BINARY_DIR})
add_dependencies(othello_engine generated_headers)
target_link_libraries(othello_engine
  PUBLIC
    fmt::fmt
    spdlog::spdlog
)

# Main executable (uses engine)
add_executable(othelloplayer src/main.cpp)
target_include_directories(othelloplayer PRIVATE ${rapidyaml_SOURCE_DIR}/src)
target_include_directories(othelloplayer PRIVATE ${CMAKE_BINARY_DIR})
add_dependencies(othelloplayer generated_headers)
target_link_libraries(othelloplayer
  PRIVATE
    othello_engine
    OpenCL::OpenCL
    spdlog::spdlog
    fmt::fmt
    ryml
    generated_headers
)

# Remote agent
add_executable(remote_agent src/remote_agent.cpp)
target_include_directories(remote_agent PRIVATE ${CMAKE_BINARY_DIR})
add_dependencies(remote_agent generated_headers)
target_link_libraries(remote_agent 
  PRIVATE 
    othello_engine
    generated_headers
)

# Unit tests
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(othello_tests ${TEST_SOURCES})
target_include_directories(othello_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(othello_tests PRIVATE ${CMAKE_BINARY_DIR})
add_dependencies(othello_tests generated_headers)
target_link_libraries(othello_tests
  PRIVATE
    othello_engine
    OpenCL::OpenCL
    spdlog::spdlog
    fmt::fmt
    gtest_main
    generated_headers
)

include(GoogleTest)
gtest_discover_tests(othello_tests)

# Training binary
add_executable(train src/train.cpp)
target_include_directories(train PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${rapidyaml_SOURCE_DIR}/src)
target_include_directories(train PRIVATE ${CMAKE_BINARY_DIR})
add_dependencies(train generated_headers)
target_link_libraries(train
  PRIVATE
    othello_engine
    OpenCL::OpenCL
    spdlog::spdlog
    fmt::fmt
    ryml
    generated_headers
)
