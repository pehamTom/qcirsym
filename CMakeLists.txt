cmake_minimum_required(VERSION 3.13)
project(qcirsym
        LANGUAGES CXX C
        DESCRIPTION "QCIRSYM - A Tool for finding symmetries in QCIR formulas in cleansed-prenex form")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#build type settings
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Add path for custom modules
list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


add_executable(${PROJECT_NAME}
                saucy/saucy.c
                saucy/saucy.h
               src/qcirsym.cpp)


# set required C++ standard and disable compiler specific extensions
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

# enable interprocedural optimization if it is supported
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
	set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC
	                           $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/saucy/>)


# set compiler flag depending on compiler
if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC /utf-8)
else()
        target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra $<$<CONFIG:DEBUG>:-Og>)
        if(NOT DEPLOY)
		# only include machine-specific optimizations when building for the host machine
		target_compile_options(${PROJECT_NAME} PUBLIC -mtune=native -march=native)
	endif ()
endif()
