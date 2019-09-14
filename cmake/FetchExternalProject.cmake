function(FetchExternalProject)
    set(PROJ_NAME ${ARGV0})
    message(STATUS "-----------------------------------------------------------")
    message(STATUS "Fetching external project: ${PROJ_NAME}")
    set(FETCH_ROOT "${CMAKE_BINARY_DIR}/external/${PROJ_NAME}")
    file(WRITE ${FETCH_ROOT}/CMakeLists.txt 
        "cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})\n"
        "project(${PROJ_NAME}-external)\n"
        "include(ExternalProject)\n"
        "ExternalProject_Add(\n"
    )
    set(BUILD_DIR .)
    set(LAST_ARG "")
    set(ExPREFIX "")
    set(PARAM_NAME "")
    set(CM_GENERATOR "")
    set(CM_BUILD_TYPE "")
    set(CM_CONFIGURATION_TYPES "")
    set(INSTALL_COMMAND "")
    set(EXTERNAL_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    if(NOT EXTERNAL_BUILD_TYPE)
        set(EXTERNAL_BUILD_TYPE Release)
    endif()
    foreach(arg IN LISTS ARGN)
        set(A "${arg}")
        #message(STATUS "Step: ${A} | ${LAST_ARG} | ${ExPREFIX}")
        if("${LAST_ARG}" STREQUAL "PREFIX")
            set(ExPREFIX "${A}")
        endif()
        if("${LAST_ARG}" STREQUAL "CMAKE_ARGS")
            file(APPEND ${FETCH_ROOT}/CMakeLists.txt "        \"-DCMAKE_INSTALL_PREFIX=${ExPREFIX}\"\n")
        endif()

        if("${LAST_ARG}" STREQUAL "-G")
            set(CM_GENERATOR "${A}")
        elseif("${A}" MATCHES "^-G(.*)?")
            set(CM_GENERATOR "${CMAKE_MATCH_1}")
        endif()

        if("${A}" MATCHES "^-DCMAKE_BUILD_TYPE=(.*)?")
            set(CM_BUILD_TYPE "${CMAKE_MATCH_1}")
        endif()
        if("${A}" MATCHES "^-DCMAKE_CONFIGURATION_TYPE=(.*)?")
            set(CM_CONFIGURATION_TYPE "${CMAKE_MATCH_1}")
        endif()
        if("${A}" MATCHES "^[A-Z_]+$")
            if(PARAM_NAME STREQUAL "CMAKE_ARGS")
                if(NOT CM_GENERATOR)
                    file(APPEND ${FETCH_ROOT}/CMakeLists.txt "        \"-G${CMAKE_GENERATOR}\"\n")
                endif()
                if(NOT CM_BUILD_TYPE)
                    file(APPEND ${FETCH_ROOT}/CMakeLists.txt "        \"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}\"\n")
                endif()
                if(NOT CM_CONFIGURATION_TYPE)
                    file(APPEND ${FETCH_ROOT}/CMakeLists.txt "        \"-DCM_CONFIGURATION_TYPE=${CM_CONFIGURATION_TYPE}\"\n")
                endif()
            endif()
            set(PARAM_NAME "${A}")
            if(PARAM_NAME STREQUAL "INSTALL_COMMAND")
                set(INSTALL_COMMAND YES)
            endif()
            file(APPEND ${FETCH_ROOT}/CMakeLists.txt "    ${A}\n")
        else()
            file(APPEND ${FETCH_ROOT}/CMakeLists.txt "        \"${A}\"\n")
        endif()
        if("${LAST_ARG}" STREQUAL "BINARY_DIR")
            set(BUILD_DIR "${A}")
        endif()
        set(LAST_ARG "${A}")
    endforeach()
    if(NOT INSTALL_COMMAND)
        file(APPEND ${FETCH_ROOT}/CMakeLists.txt "    INSTALL_COMMAND \"${CMAKE_COMMAND}\" --build . --target install --config ${CMAKE_BUILD_TYPE}\n")
    endif()
    file(APPEND ${FETCH_ROOT}/CMakeLists.txt ")\n")
    file(MAKE_DIRECTORY "${ExPREFIX}/${PROJ_NAME}-build")
    execute_process(
        COMMAND ${CMAKE_COMMAND} 
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CONFIGURATION_TYPES=${EXTERNAL_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_INSTALL_PREFIX=${FETCH_ROOT}
            ".."
        WORKING_DIRECTORY "${ExPREFIX}/${PROJ_NAME}-build"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(result EQUAL "0")
        message(STATUS "Pre-fetch configuration done")
    else()
        message(STATUS "result='${result}'")
        message(STATUS "stdout='${stdout}'")
        message(STATUS "stderr='${stderr}'")
        message(FATAL_ERROR "Failed configuration of ${PROJ_NAME}")
    endif()
    execute_process(
        COMMAND ${CMAKE_COMMAND} 
            --build .
            --config ${EXTERNAL_BUILD_TYPE}
        WORKING_DIRECTORY "${ExPREFIX}/${PROJ_NAME}-build"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(result EQUAL "0")
        message(STATUS "Fetch and compilation done")
    else()
        message(STATUS "result='${result}'")
        message(STATUS "stdout='${stdout}'")
        message(STATUS "stderr='${stderr}'")
        message(FATAL_ERROR "Failed to fetch and build ${PROJ_NAME}")
    endif()
    string(TOUPPER ${PROJ_NAME} PROJ_UPPERCASE_NAME)
    set(${PROJ_UPPERCASE_NAME}_INCLUDE_DIR "${FETCH_ROOT}/include" PARENT_SCOPE)
    set(${PROJ_UPPERCASE_NAME}_LIBRARY_DIR "${FETCH_ROOT}/lib" PARENT_SCOPE)
    list(APPEND CMAKE_PREFIX_PATH "${FETCH_ROOT}")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
    message(STATUS "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -")
endfunction()
