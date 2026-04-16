message("** CMakeBoostTest ..... Loading")

include(CMakeParseArguments)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

enable_testing()

function (boost_test_project)

	cmake_parse_arguments(
        PARSED_ARGS # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "NAME" # list of names of mono-valued arguments
        "DEPS" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )

    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "You must provide a name")
    endif(NOT PARSED_ARGS_NAME)

	set(BOOST_ROOT "C:/boost_1_90_0")

	if(Boost_FOUND)
  		include_directories(${Boost_INCLUDE_DIRS})
  	else()
  		message("Boost was not found !!!")
  	endif()

	message("--> Creating boost test project: " ${PARSED_ARGS_NAME})


	project (${PARSED_ARGS_NAME})
	include_directories(..)

	file(GLOB SRC_FILES  ${CXX_FILE_EXTENSIONS})
	add_executable(${PARSED_ARGS_NAME}  ${SRC_FILES})
	target_link_libraries(${PARSED_ARGS_NAME} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

	foreach(lib_dep ${PARSED_ARGS_DEPS})
        target_link_libraries(${PARSED_ARGS_NAME} ${lib_dep})
    endforeach(lib_dep)

	# Add each test case to CTest independently, idea taken from
	# https://eb2.co/blog/2015/06/driving-boost-dot-test-with-cmake/
	#but with support for test suite.
	#We are assuming we have always ONE test suite not more but not less.


    foreach(test_file ${SRC_FILES})

		file(READ "${test_file}" SOURCE_FILE_CONTENTS)

		string(REGEX MATCHALL "BOOST_AUTO_TEST_SUITE\\( *([A-Za-z_0-9]+) *\\)"
	           FOUND_TESTS_SUITE ${SOURCE_FILE_CONTENTS})

		string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" TEST_SUITE_NAME ${FOUND_TESTS_SUITE})


	    string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\( *([A-Za-z_0-9]+) *\\)"
	           FOUND_TESTS ${test_file})

	    foreach(HIT ${FOUND_TESTS})
	        string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" TEST_NAME ${HIT})

	 		message("found test: " ${PARSED_ARGS_NAME}.${TEST_NAME})
	        add_test(NAME "${PARSED_ARGS_NAME}.${TEST_NAME}"
	                 COMMAND ${CMAKE_BINARY_DIR}/${PARSED_ARGS_NAME}
	                 --run_test=${TEST_SUITE_NAME}/${TEST_NAME} --catch_system_error=yes)
	        # other way: --run_test=*/${TEST_NAME}
	    endforeach(HIT)
	endforeach(test_file)
endfunction(boost_test_project)
message("** CMakeBoostTest ..... Loaded")
