
include(FindPkgConfig)
pkg_check_modules(RTAUDIO_PC QUIET rtaudio)

#message(STATUS "Variables:")
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

if(RTAUDIO_PC_FOUND)
    set(RTAUDIO_LIBRARY_DIRS ${RTAUDIO_PC_LIBRARY_DIRS})
    #set(RTAUDIO_INCLUDE_DIR ${RTAUDIO_PC_INCLUDEDIR})
    set(RTAUDIO_VERSION_STRING ${RTAUDIO_PC_VERSION})
    if(NOT WIN32)
    if(RTAUDIO_BUILD_STATIC_LIBS OR NOT BUILD_SHARED_LIBS)
	    set (RTAUDIO_LIBRARIES ${RTAUDIO_PC_STATIC_LIBRARIES})
    else()
	    set (RTAUDIO_LIBRARIES ${RTAUDIO_PC_LIBRARIES})
    endif()
    endif()
endif()

if(WIN32)
find_path(
  RTAUDIO_INCLUDE_DIR RtAudio.h
  HINTS ${RTAUDIO_PC_INCLUDEDIR}
)

find_library(
  RTAUDIO_LIBRARIES NAMES rtaudio.lib
  HINTS ${RTAUDIO_PC_LIBRARY_DIRS}
)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    RtAudio
    REQUIRED_VARS RTAUDIO_LIBRARIES RTAUDIO_INCLUDE_DIR
    VERSION_VAR RTAUDIO_VERSION_STRING
)
