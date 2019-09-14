####################################################
#
#  FILE:
#  CommonSetup.cmake
#
#  BSD 3-Clause License
#  --------------------
#  
#  Copyright (c) 2017 Steffen Schümann, all rights reserved.
#  
#  1. Redistribution and use in source and binary forms, with or without
#     modification, are permitted provided that the following conditions are met:
#  
#  2. Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#  
#  3. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution. Neither the name of
#     the copyright holder nor the names of its contributors may be used to endorse
#     or promote products derived from this software without specific prior written
#     permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
#  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###########################################################################

if(POLICY CMP0046)
cmake_policy(SET CMP0046 NEW)
endif()

if(POLICY CMP0063)
cmake_policy(SET CMP0063 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT CMAKE_CXX_STANDARD)
    # ensure at least C++11 is active
    set(CMAKE_CXX_STANDARD 11)
endif()

message("Configuring C++ compiler to use C++${CMAKE_CXX_STANDARD} ...")

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN True)


find_package(Git REQUIRED)

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TOLOWER ${PROJECT_NAME} PROJECT_LOWERCASE_NAME)
string(TOUPPER ${PROJECT_NAME} PROJECT_UPPERCASE_NAME)

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE PROJECT_VERSION_PATCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(${PROJECT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/version/version.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
    set(SYSTEM_LIBS "")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wheader-hygiene -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wmissing-declarations -Wmissing-prototypes -Woverlength-strings -Wshadow -Wunreachable-code -Wextra -Wno-unused-parameter -Wpedantic -Werror")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(SYSTEM_LIBS pthread)
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # we are using Clang under Linux
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wheader-hygiene -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wmissing-declarations -Wmissing-prototypes -Woverlength-strings -Wshadow -Wunreachable-code -Wextra -Wno-unused-parameter -Wpedantic -Werror")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # we are using GCC under Linux
            execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
            if(GCC_VERSION VERSION_LESS 4.7)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
                message(FATAL_ERROR "To compile with GCC, a version of 4.7 or newer is needed!")
            endif()
        else()
            message(FATAL_ERROR "Sorry, I couldn't recognize the compiler, so I don't know how to configure it!")
        endif()
    endif()
endif(UNIX)
if(WIN32)
	add_definitions(-DNOMINMAX)
    set(SYSTEM_LIBS "")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

