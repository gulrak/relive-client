cmake_minimum_required(VERSION 3.10)
project(reLive VERSION 0.4)

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
option(BUILD_WITH_TESTS "Compile test too" ON)

find_package(Threads REQUIRED)

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

if(USE_RTAUDIO_BACKEND)
    set(RTAUDIO_PREFIX ${CMAKE_BINARY_DIR}/external/rtaudio)
    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(ALSA_BACKEND OFF)
    elseif(UNIX AND NOT APPLE)
        set(ALSA_BACKEND ON)
    else()
        set(ALSA_BACKEND OFF)
    endif()
    FetchExternalProject(rtaudio
        PREFIX "${RTAUDIO_PREFIX}"
        GIT_REPOSITORY "https://github.com/thestk/rtaudio.git"
        GIT_TAG "1cba5c90a35b0e79915dc46dd5525da2285a211b"
        CMAKE_ARGS
            "-DRTAUDIO_BUILD_STATIC_LIBS=ON"
            "-DRTAUDIO_BUILD_TESTING=OFF"
            "-DRTAUDIO_API_PULSE=OFF"
            "-DRTAUDIO_API_JACK=OFF"
            "-DRTAUDIO_API_ALSA=${ALSA_BACKEND}"
    )
    find_package(RtAudio REQUIRED)
    include_directories(${RTAUDIO_INCLUDE_DIR})
    link_directories(${RTAUDIO_PC_LIBRARY_DIRS})
    set(AUDIO_BACKEND_LIBRARIES ${RTAUDIO_LIBRARIES} "-framework CoreAudio -framework CoreFoundation")
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

include_directories(src thirdparty)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src/backend)
add_subdirectory(src/frontend)

if(BUILD_WITH_TESTS)
    add_subdirectory(test)
endif()
