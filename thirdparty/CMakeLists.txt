set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

add_executable(binary_to_compressed_c imgui/misc/fonts/binary_to_compressed_c.cpp)

set(IMGUI_SOURCES
        imgui/examples/imgui_impl_glfw.cpp
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui.h
        imgui/imgui_internal.h
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
    target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
    target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/glfw3/include ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/libs/gl3w)
    target_link_libraries(imgui PUBLIC "-framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore -L/usr/local/lib -lglfw")
else()
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W IMGUI_DEFINE_MATH_OPERATORS)
    target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/libs/gl3w)
    target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})
endif()

#add_subdirectory(rtaudio)
#add_subdirectory(midifile)
