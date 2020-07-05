
set(IMGUIX_SOURCES application.h applog.h resources.cpp resources.h logging.cpp logging.h curveeditor.h verticaltab.h utility.h knob.h knob.cpp gradient.cpp gradient.h imguix.h imguix.cpp)

if(APPLE AND NOT PREFER_OPENGL3_BACKEND)
    list(APPEND IMGUIX_SOURCES application.mm)
else()
    list(APPEND IMGUIX_SOURCES application.cpp)
endif()

add_library(imguix ${IMGUIX_SOURCES})
target_link_libraries(imguix PUBLIC imgui)