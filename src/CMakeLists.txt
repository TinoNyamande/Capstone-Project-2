cmake_minimum_required(VERSION 3.15)

# Project name and version
project(tino LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

if (LLVM_FOUND)
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()


# Add LLVM's include directories
include_directories( 
${PROJECT_SOURCE_DIR}/include
${PROJECT_SOURCE_DIR}/lexer
${PROJECT_SOURCE_DIR}/ast
${PROJECT_SOURCE_DIR}/parser
${PROJECT_SOURCE_DIR}/codegen)
add_definitions(${LLVM_DEFINITIONS})

# List of LLVM components needed
set(LLVM_LINK_COMPONENTS
  Core
  ExecutionEngine
  Object
  OrcJIT
  Support
  TargetParser
  native
)

# Add the toy.cpp file to the executable
add_executable(tino
  lexer/lexer.cpp
  ast/ast.cpp
  parser/parser.cpp
  codegen/codegen.cpp
  main.cpp
)

# Ensure LLVM's CMake files are loaded
llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_LINK_COMPONENTS})

# Link against LLVM libraries
target_link_libraries(tino PRIVATE ${LLVM_LIBS})

# Specify the include paths for the toy.cpp dependencies
include_directories(include)

# Print LLVM information
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
