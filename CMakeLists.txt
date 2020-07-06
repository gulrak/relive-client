cmake_minimum_required(VERSION 3.10)
project(reLive VERSION 0.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CreateVersion)
include(FetchExternalProject)

option(USE_RTAUDIO_BACKEND "Use RtAudio as audio backend." ON)
option(USE_PORTAUDIO_BACKEND "Use PortAudio as audio backend." OFF)
option(USE_OPENSSL "Use OpenSSL instead ot LibreSSL" OFF)
option(USE_SYSTEM_SQLITE3 "Use system provided sqlite3 libs" OFF)
option(BUILD_WITH_TESTS "Compile test too" OFF)
option(USE_IMGUI_FRONTEND "Build IMGUI based frontend." ON)
option(PREFER_OPENGL3_BACKEND "When active, use OpenGL3 where possible" ON)

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
        URL "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.0.0.tar.gz"
        URL_HASH SHA1=8c63219bb6be84b75b9ffed16ca419e1ed7efbdd
        CMAKE_ARGS
            "-DLIBRESSL_APPS=OFF"
            "-DLIBRESSL_TESTS=OFF"
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
    set(AUDIO_BACKEND_LIBRARIES ${PORTAUDIO_LIBRARIES} "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework Carbon")
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

if(XUSE_IMGUI_FRONTEND)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW3_PREFIX ${CMAKE_BINARY_DIR}/external/glfw3)
    FetchExternalProject(glfw3
        PREFIX "${GLFW3_PREFIX}"
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        GIT_TAG "3.3.2"
        CMAKE_ARGS
            "-DGLFW_BUILD_EXAMPLES=OFF"
            "-DGLFW_BUILD_TESTS=OFF"
            "-DGLFW_BUILD_DOCS=OFF"
            "-DGLFW_INSTALL=OFF"
        INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
    )
    include_directories(${GLFW3_PREFIX}/src/glfw3/include)
    set(IMGUI_PREFIX ${CMAKE_BINARY_DIR}/external/imgui)
    FetchExternalProject(imgui
        PREFIX "${IMGUI_PREFIX}"
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG "v1.77"
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/CMakeLists.txt" "${IMGUI_PREFIX}/src/imgui/CMakeLists.txt"
        INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo ""
    )
    include_directories(${IMGUI_PREFIX}/src ${IMGUI_PREFIX}/src/imgui frontend/imgui)
endif()

include_directories(src thirdparty thirdparty/rtaudio)

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
endif()

add_subdirectory(src/backend)
add_subdirectory(src/frontend)

if(BUILD_WITH_TESTS)
    add_subdirectory(test)
endif()

include(CPack)

