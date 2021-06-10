
set(PARSE_CATCH_TESTS_ADD_TO_CONFIGURE_DEPENDS ON)
include(ParseAndAddCatchTests)

add_executable(relive-test relivedb_tests.cpp helper.hpp)
target_link_libraries(relive-test relive-backend ${SSL_BACKEND} ${AUDIO_BACKEND_LIBRARIES} ${SQLITE3_TARGET} Threads::Threads)
ParseAndAddCatchTests(relive-test)

add_executable(player-test player_test.cpp)
target_link_libraries(player-test relive-backend ${SSL_BACKEND} ${AUDIO_BACKEND_LIBRARIES} ${SQLITE3_TARGET} Threads::Threads)
ParseAndAddCatchTests(player-test)

