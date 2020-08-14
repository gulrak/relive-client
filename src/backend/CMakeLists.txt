

set(RELIVE_BACKEND_SOURCE
    logging.cpp
    player.cpp
    relivedb.cpp
    system.cpp
)
set(RELIVE_BACKEND_HEADER
    logging.hpp
    player.hpp
    relivedb.hpp
    ringbuffer.hpp
    rldata.hpp
    system.hpp
)
set(RELIVE_BACKEND_THIRDPARTY
    ../../thirdparty/ghc/filesystem.hpp
    ../../thirdparty/ghc/uri.hpp
    ../../thirdparty/nlohmann/json.hpp
    ../../thirdparty/sqlite_orm/sqlite_orm.h
)
add_library(relive-backend STATIC ${RELIVE_BACKEND_SOURCE} ${RELIVE_BACKEND_HEADER} ${RELIVE_BACKEND_THIRDPARTY} ${SQLITE3_SOURCES})
set_source_files_properties(relivedb.cpp PROPERTIES COMPILE_FLAGS "-O0")
target_compile_options(relive-backend PRIVATE -DSQLITE_OMIT_LOAD_EXTENSION)
target_link_libraries(relive-backend PRIVATE ${SSL_BACKEND})

