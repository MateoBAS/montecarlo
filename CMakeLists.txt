cmake_minimum_required (VERSION 3.10)

project (sqf)

# 1. Define some well-known folders:
# - Place for compile binaries
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
# - Root for include files
set(INCLUDE_HOME ${PROJECT_SOURCE_DIR}/src)
# - In-house cmake functions
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-lib)

# - Configuración completa de Boost para MinGW en Windows
set(BOOST_ROOT "C:/boost_1_81_0")
set(BOOST_LIBRARYDIR "C:/boost_1_81_0/stage/lib") # Dónde están los binarios
set(Boost_ARCHITECTURE "-x64")                    # Arquitectura de MinGW
set(Boost_USE_STATIC_LIBS ON)                     # Buscar archivos .a

set(EIGEN_ROOT "C:/eigen-5.0.0")


# Print some stuff
message("--> CMake extra functions: " ${CMAKE_MODULE_PATH})
message("--> Binaries: " ${CMAKE_BINARY_DIR})
message("--> Include base path: " ${INCLUDE_HOME})
message("--> Eigen path: " ${EIGEN_ROOT})


# 2. Load in-house cmake modules
message("--> Loading CMake modules: ")
include(${CMAKE_MODULE_PATH}/CMakeBoostTest.cmake)
include(${CMAKE_MODULE_PATH}/CMakeAuxFunctions.cmake)


# 3. Set CMake environment
# - Set folders
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# - C++ file filter for adding files in cmake's projects
set(CXX_FILE_EXTENSIONS "*.h"  "*.cpp")

# 4. Set Compilation environment
include_directories(${INCLUDE_HOME} ${EIGEN_ROOT})
add_definitions(-std=gnu++14)

# 5. Add subdirs
add_subdirectory(src)
