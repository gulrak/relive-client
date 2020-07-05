include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}/external/imgui/src/imgui/examples/libs/gl3w)
add_definitions(-DGL_SILENCE_DEPRECATION -DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_IMPL_OPENGL_LOADER_GL3W)
add_subdirectory(resources)
add_subdirectory(imguix)

add_executable(reLiveG MACOSX_BUNDLE
    reliveapp.cpp
)

target_link_libraries(reLiveG imguix resourcedata relive-backend ${SSL_BACKEND} ${AUDIO_BACKEND_LIBRARIES} ${SQLITE3_TARGET} Threads::Threads)

if(APPLE)
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in")
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME reLiveG)
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.gulrak.reliveg")
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION})
endif()
