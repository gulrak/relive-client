include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}/external/imgui/src/imgui/examples/libs/gl3w)
add_definitions(-DGL_SILENCE_DEPRECATION -DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_IMPL_OPENGL_LOADER_GL3W)
add_subdirectory(resources)
add_subdirectory(imguix)

set(MACOSX_BUNDLE_ICON_FILE reLiveG.icns)
set(reLiveG_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/${MACOSX_BUNDLE_ICON_FILE})
set_source_files_properties(${reLiveG_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
add_executable(reLiveG MACOSX_BUNDLE ${reLiveG_ICON}
    reliveapp.hpp
    reliveapp.cpp
    reliveapp_menu.cpp
    stylemanager.h
    ${TRACY_CPP}
)

target_link_libraries(reLiveG imguix resourcedata relive-backend ${SSL_BACKEND} ${AUDIO_BACKEND_LIBRARIES} ${SQLITE3_TARGET} Threads::Threads)

if(APPLE)
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in")
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME reLiveG)
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.gulrak.reliveg")
    set_target_properties(reLiveG PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION})
endif()

if(APPLE)
    install(TARGETS relivecui DESTINATION "." COMPONENT relivecui_app)
    install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION "." COMPONENT relivecui_app)
else()
    install(TARGETS relivecui DESTINATION bin COMPONENT relivecui_app)
endif()
if(UNIX AND NOT APPLE)
    install(FILES ${PROJECT_SOURCE_DIR}/man/relivecui.1 DESTINATION shared/man/man1 COMPONENT relivecui_app)
endif()