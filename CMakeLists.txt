cmake_minimum_required(VERSION 3.10)
#set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")
project(reLive VERSION 0.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CreateVersion)
include(FetchExternalProject)

option(USE_MINIAUDIO_BACKEND "Use MiniAudio as audio backend." ON)
option(USE_RTAUDIO_BACKEND "Use RtAudio as audio backend." OFF)
option(USE_PORTAUDIO_BACKEND "Use PortAudio as audio backend." OFF)
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

if(USE_OPENSSL)
    if(APPLE)
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@1.1")
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSS_INCLUDE_DIRS})
    set(SSL_BACKEND OpenSSL::SSL)
else()
    set(LIBRESSL_PREFIX ${CMAKE_BINARY_DIR}/external/libressl)
    FetchExternalProject(libressl
        PREFIX "${LIBRESSL_PREFIX}"
        URL "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.3.1.tar.gz"
        URL_HASH SHA1=b86b9f566efe3b3a43cf4796af1ab719b4d0f63f
        CMAKE_ARGS
            "-DLIBRESSL_APPS=OFF"
            "-DLIBRESSL_TESTS=OFF"
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.12"
    )
    find_package(LibreSSL REQUIRED)
    include_directories(${LIBRESSL_INCLUDE_DIR})
    set(SSL_BACKEND LibreSSL::SSL)
endif()

if(USE_SYSTEM_SQLITE3)
    find_package(SQLite3 REQUIRED)
    set(SQLITE3_SOURCES "")
    set(SQLITE3_TARGET SQLite::SQLite3)
else()
    set(SQLITE3_PREFIX ${CMAKE_BINARY_DIR}/external/sqlite3)
    FetchExternalProject(sqlite3
        PREFIX "${SQLITE3_PREFIX}"
        URL "https://www.sqlite.org/2019/sqlite-amalgamation-3290000.zip"
        URL_HASH SHA1=a0eba79e5d1627946aead47e100a8a6f9f6fafff
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
    )
    set(SQLITE3_SOURCES "${SQLITE3_PREFIX}/src/sqlite3/sqlite3.c" "${SQLITE3_PREFIX}/src/sqlite3/sqlite3.h")
    include_directories("${SQLITE3_PREFIX}/src/sqlite3")
    set(SQLITE3_TARGET "")
endif()

if(USE_PORTAUDIO_BACKEND)
    find_package(PortAudio REQUIRED)
    include_directories(${PORTAUDIO_LIBRARIES})
    if(APPLE)
        set(AUDIO_BACKEND_LIBRARIES ${PORTAUDIO_LIBRARIES} "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework Carbon")
    else()
        set(AUDIO_BACKEND_LIBRARIES ${PORTAUDIO_LIBRARIES})
    endif()
    add_definitions(-DRELIVE_PORTAUDIO_BACKEND)
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
	    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pdcurses/CMakeLists.txt" "${PDCURSES_PREFIX}/src/PDCurses/CMakeLists.txt"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
    )
    add_subdirectory("${PDCURSES_PREFIX}/src/PDCurses" "${PDCURSES_PREFIX}/src/pdcurses-build")
endif()

include_directories(BEFORE src thirdparty thirdparty/rtaudio)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(tools)
add_subdirectory(thirdparty)
if(USE_RTAUDIO_BACKEND)
    if(APPLE)
        set(AUDIO_BACKEND_LIBRARIES rtaudio "-framework CoreAudio -framework CoreFoundation")
    else()
        set(AUDIO_BACKEND_LIBRARIES rtaudio)
    endif()
    add_definitions(-DRELIVE_RTAUDIO_BACKEND)
endif()

if(USE_MINIAUDIO_BACKEND)
    if(APPLE)
        set(AUDIO_BACKEND_LIBRARIES "-framework CoreAudio -framework AudioUnit -framework CoreFoundation")
    endif()
    add_definitions(-DRELIVE_MINIAUDIO_BACKEND -DMA_ENABLE_ONLY_SPECIFIC_BACKENDS -DMA_ENABLE_COREAUDIO)
endif()

add_subdirectory(src/backend)
add_subdirectory(src/frontend)

if(BUILD_WITH_TESTS)
    add_subdirectory(test)
endif()

include(CPack)

