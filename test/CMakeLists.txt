
add_executable(relive-test relivedb_tests.cpp helper.hpp)
target_link_libraries(relive-test relive-backend ${SSL_BACKEND} ${AUDIO_BACKEND_LIBRARIES} ${SQLITE3_TARGET} Threads::Threads)

