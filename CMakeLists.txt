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

# Define source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Targets
add_executable(othelloplayer ${SOURCES})
add_dependencies(othelloplayer embed_weights)
target_include_directories(othelloplayer PRIVATE ${CMAKE_BINARY_DIR}/generated)

add_executable(train src/train.cpp)
target_link_libraries(train
  PRIVATE
    OpenCL::OpenCL
    spdlog::spdlog
    ryml
)

# Add include directories
target_include_directories(othelloplayer
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${rapidyaml_SOURCE_DIR}/src
)

target_include_directories(train
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${rapidyaml_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(othelloplayer
  PRIVATE
    OpenCL::OpenCL
    spdlog::spdlog
    ryml
)

# Paths
set(MODEL_INPUT ${CMAKE_SOURCE_DIR}/model/weights.bin)
set(MODEL_HEADER ${CMAKE_BINARY_DIR}/generated/weights.hpp)

add_executable(embed_model tools/embed_model.cpp)

add_custom_command(
  OUTPUT ${MODEL_HEADER}
  COMMAND embed_model ${MODEL_INPUT} ${MODEL_HEADER}
  DEPENDS embed_model ${MODEL_INPUT}
  COMMENT "Embedding weights into header"
)

# Ensure the header is generated before building the main app
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
add_custom_target(embed_weights DEPENDS ${MODEL_HEADER})