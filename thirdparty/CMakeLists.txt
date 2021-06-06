find_package(Git QUIET)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if (GIT_SUBMODULE)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/glfw/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

if(USE_RTAUDIO_BACKEND)
    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(ALSA_BACKEND OFF)
    elseif(UNIX AND NOT APPLE)
        set(ALSA_BACKEND ON)
    else()
        set(ALSA_BACKEND OFF)
    endif()
    set(RTAUDIO_API_ASIO OFF CACHE INTERNAL "do not use asio")
    set(RTAUDIO_API_DS ${WIN32} CACHE INTERNAL "use ds on windows")
    set(RTAUDIO_API_WASAPI ${WIN32} CACHE INTERNAL "use wasapi on windows")
    set(RTAUDIO_API_OSS ${xBSD} CACHE INTERNAL "use oss on BSD")
    set(RTAUDIO_API_ALSA ${ALSA_BACKEND} CACHE INTERNAL "selective use alsa")
    set(RTAUDIO_API_PULSE OFF CACHE INTERNAL "dont use pulse")
    set(RTAUDIO_API_JACK OFF CACHE INTERNAL "dont use jack")
    set(RTAUDIO_API_CORE ${APPLE} CACHE INTERNAL "use OSX_CORE!")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "dont build shared libs")
    set(BUILD_TESTING OFF CACHE INTERNAL "dont build tests!")
    add_subdirectory(rtaudio)
endif()

add_executable(binary_to_compressed_c imgui/misc/fonts/binary_to_compressed_c.cpp)

set(IMGUI_SOURCES
        imgui/examples/imgui_impl_glfw.cpp
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui.h
        imgui/imgui_internal.h
        imgui/misc/cpp/imgui_stdlib.cpp
        imgui/misc/cpp/imgui_stdlib.h
)

if(PREFER_OPENGL3_BACKEND)
    find_package(OpenGL REQUIRED)
endif()

if(APPLE AND NOT PREFER_OPENGL3_BACKEND)
    list(APPEND IMGUI_SOURCES imgui/examples/imgui_impl_metal.mm imgui/examples/imgui_impl_metal.h)
else()
    list(APPEND IMGUI_SOURCES imgui/examples/imgui_impl_opengl3.cpp imgui/examples/imgui_impl_opengl3.h imgui/examples/libs/gl3w/GL/gl3w.c)
endif()

add_library(imgui ${IMGUI_SOURCES})

if(APPLE AND NOT PREFER_OPENGL3_BACKEND)
    target_compile_options(imgui PUBLIC -fobjc-arc)
    target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32 IMGUI_DEFINE_MATH_OPERATORS)
    target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/glfw3/include ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/libs/gl3w)
    target_link_libraries(imgui PUBLIC "-framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore -L/usr/local/lib -lglfw")
else()
    target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32 IMGUI_IMPL_OPENGL_LOADER_GL3W IMGUI_DEFINE_MATH_OPERATORS)
    target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/libs/gl3w)
    target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})
endif()
