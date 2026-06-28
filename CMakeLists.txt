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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-O3 -march=native -g)

# 5. Add subdirs
add_subdirectory(src)

# --- TESTS GLOBALES DE INTEGRACIÓN ---
# 1. Creamos el ejecutable
add_executable(integration_tests 
    src/Integration/test_main.cpp 
    src/Integration/test_acciones.cpp 
)

target_include_directories(integration_tests PRIVATE ${BOOST_ROOT})

target_link_libraries(integration_tests Asset Portfolio Maths Metrics Random MontecarloEngine Boost::unit_test_framework)

# Tests ligeros: validación rápida del refactor simulateFinalValue / generatePath
add_executable(light_tests
    src/LightTests/test_light.cpp
)
target_include_directories(light_tests PRIVATE ${BOOST_ROOT})
target_link_libraries(light_tests Asset Portfolio Metrics Boost::unit_test_framework)
add_test(NAME light_tests COMMAND light_tests)

add_executable(standard_error_tests
    src/LightTests/test_standard_errors.cpp
)
target_include_directories(standard_error_tests PRIVATE ${BOOST_ROOT})
target_link_libraries(standard_error_tests Metrics Boost::unit_test_framework)
add_test(NAME standard_error_tests COMMAND standard_error_tests)

add_executable(antithetic_tests
    src/LightTests/test_antithetic.cpp
)
target_include_directories(antithetic_tests PRIVATE ${BOOST_ROOT})
target_link_libraries(antithetic_tests Asset Portfolio Metrics MontecarloEngine Boost::unit_test_framework)
add_test(NAME antithetic_tests COMMAND antithetic_tests)

# Benchmark aislado: localidad ColMajor vs RowMajor (sin dependencias del motor)
add_executable(layout_benchmark_tests
    src/LightTests/test_matrix_layout.cpp
)
target_include_directories(layout_benchmark_tests PRIVATE ${BOOST_ROOT} ${EIGEN_ROOT})
target_link_libraries(layout_benchmark_tests Boost::unit_test_framework)
add_test(NAME layout_benchmark_tests COMMAND layout_benchmark_tests)
