
add_executable(resourcer resourcer.cpp)
add_executable(codepointuse codepointuse.cpp unicode-blocks.h)
target_link_libraries(codepointuse relive-backend)
