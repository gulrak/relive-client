cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")
project(reLive VERSION 0.7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CreateVersion)
include(FetchExternalProject)

option(USE_OPENSSL "Use OpenSSL instead ot LibreSSL" ON)
option(USE_SYSTEM_SQLITE3 "Use system provided sqlite3 libs" ON)
option(BUILD_WITH_TESTS "Compile test too" OFF)
option(USE_IMGUI_FRONTEND "Build IMGUI based frontend." ON)
option(PREFER_OPENGL3_BACKEND "When active, use OpenGL3 where possible" ON)
set(TRACY_PATH "" CACHE STRING "Path to tracy profiler, enables profiling")

if(NOT BUILD_WITH_TESTS)
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
endif()

if(UNIX AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "..." FORCE)
endif()

find_package(Threads REQUIRED)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "(Open|Free|Net)BSD")
    set(xBSD ON)
endif()

if(TRACY_PATH)
    add_definitions(-DTRACY_ENABLE)
    include_directories("${TRACY_PATH}")
    set(TRACY_CPP "${TRACY_PATH}/TracyClient.cpp")
endif()


if(CMAKE_CXX_PLATFORM_ID MATCHES "MinGW")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj")
endif()

include(FetchContent)

if(USE_OPENSSL)
    if(APPLE)
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@1.1")
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        include_directories(${OPENSS_INCLUDE_DIRS})
        set(SSL_BACKEND OpenSSL::SSL)
    endif()
endif()
if(NOT OPENSSL_FOUND OR NOT USE_OPENSSL)
    FetchContent_Declare(libressl
        URL               https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.3.1.tar.gz
        URL_HASH SHA1=b86b9f566efe3b3a43cf4796af1ab719b4d0f63f
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/libressl-src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/libressl-build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        CMAKE_ARGS ""
        )
    FetchContent_GetProperties(libressl)
    if(NOT libressl_POPULATED)
        set(LIBRESSL_SKIP_INSTALL ON CACHE BOOL "Skip installation" )
        set(LIBRESSL_APPS OFF CACHE BOOL "Build apps")
        set(LIBRESSL_TESTS OFF CACHE BOOL "Build tests")
        FetchContent_Populate(libressl)
        add_subdirectory(${libressl_SOURCE_DIR} ${libressl_BINARY_DIR})
        include_directories(${libressl_SOURCE_DIR}/include)
    endif()
endif()

if(USE_SYSTEM_SQLITE3)
    find_package(SQLite3)
    if(SQLite3_FOUND)
        set(SQLITE3_SOURCES "")
        set(SQLITE3_TARGET SQLite::SQLite3)
    endif()
endif()
if(NOT SQLite3_FOUND OR NOT USE_SYSTEM_SQLITE3)
    FetchContent_Declare(sqlite3
        URL               https://www.sqlite.org/2019/sqlite-amalgamation-3290000.zip
        URL_HASH SHA1=a0eba79e5d1627946aead47e100a8a6f9f6fafff
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/sqlite3-src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/sqlite-build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        CMAKE_ARGS ""
        )
    FetchContent_GetProperties(sqlite3)
    if(NOT sqlite3_POPULATED)
        FetchContent_Populate(sqlite3)
        set(SQLITE3_SOURCES "${sqlite3_SOURCE_DIR}/sqlite3.c" "${sqlite3_SOURCE_DIR}/sqlite3.h")
        include_directories("${sqlite3_SOURCE_DIR}")
        set(SQLITE3_TARGET "")
    endif()
endif()

set(HTTPLIB_PREFIX ${CMAKE_BINARY_DIR}/external/httplib)
FetchExternalProject(httplib
    PREFIX "${HTTPLIB_PREFIX}"
    GIT_REPOSITORY "https://github.com/gulrak/cpp-httplib.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
)
include_directories(${HTTPLIB_PREFIX}/src/httplib)

set(MINIMP3_PREFIX ${CMAKE_BINARY_DIR}/external/minimp3)
FetchExternalProject(minimp3
    PREFIX "${MINIMP3_PREFIX}"
    GIT_REPOSITORY "https://github.com/lieff/minimp3.git"
    GIT_TAG "977514a6dfc4960d819a103f43b358e58ac6c28f"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
)
include_directories(${MINIMP3_PREFIX}/src/minimp3)

if(WIN32)
    set(PDCURSES_PREFIX ${CMAKE_BINARY_DIR}/external/pdcurses)
    FetchExternalProject(pdcurses
        PREFIX "${PDCURSES_PREFIX}"
        GIT_REPOSITORY "https://github.com/wmcbrine/PDCurses.git"
        GIT_TAG "3.9"
	    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pdcurses/CMakeLists.txt" "${PDCURSES_PREFIX}/src/pdcurses/CMakeLists.txt"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
    )
    add_subdirectory("${PDCURSES_PREFIX}/src/pdcurses" "${PDCURSES_PREFIX}/src/pdcurses-build")
endif()

include_directories(BEFORE src thirdparty)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(tools)
add_subdirectory(thirdparty)

if(APPLE)
    set(AUDIO_BACKEND_LIBRARIES "-framework CoreAudio -framework AudioUnit -framework CoreFoundation")
    add_definitions(-DMA_ENABLE_ONLY_SPECIFIC_BACKENDS -DMA_ENABLE_COREAUDIO)
elseif(UNIX)
    add_definitions(-DMA_ENABLE_ONLY_SPECIFIC_BACKENDS -DMA_ENABLE_ALSA)
elseif(WIN32)
    add_definitions(-DMA_ENABLE_ONLY_SPECIFIC_BACKENDS -DMA_ENABLE_WASAPI)
endif()

add_subdirectory(src/backend)
add_subdirectory(src/frontend)

if(BUILD_WITH_TESTS)
    add_subdirectory(test)
endif()

include(CPack)

