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

set(HOST_OS "${CMAKE_HOST_SYSTEM_NAME}")
if(UNIX)
    if(APPLE)
        set(HOST_OS "Darwin")
    else()
        find_program(LSB_RELEASE_EXEC lsb_release)
        if(LSB_RELEASE_EXEC)
            execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
                OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process(COMMAND ${LSB_RELEASE_EXEC} -cs
                OUTPUT_VARIABLE LSB_RELEASE_NAME_SHORT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(HOST_OS "${LSB_RELEASE_NAME_SHORT}")
        endif()
    endif()
endif()
