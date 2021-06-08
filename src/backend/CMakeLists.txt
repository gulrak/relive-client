

set(RELIVE_BACKEND_SOURCE
    hash.cpp
    logging.cpp
    player.cpp
    relivedb.cpp
    rldata.cpp
    system.cpp
)
set(RELIVE_BACKEND_HEADER
    hash.hpp
    logging.hpp
    player.hpp
    relivedb.hpp
    ringbuffer.hpp
    rldata.hpp
    system.hpp
    utility.hpp
)
set(RELIVE_BACKEND_THIRDPARTY
    ../../thirdparty/ghc/filesystem.hpp
    ../../thirdparty/ghc/uri.hpp
    ../../thirdparty/nlohmann/json.hpp
    ../../thirdparty/sqlite_orm/sqlite_orm.h
)
add_library(relive-backend STATIC ${RELIVE_BACKEND_SOURCE} ${RELIVE_BACKEND_HEADER} ${RELIVE_BACKEND_THIRDPARTY} ${SQLITE3_SOURCES})
set_source_files_properties(relivedb.cpp PROPERTIES COMPILE_FLAGS "-O1")
target_compile_options(relive-backend PRIVATE -DSQLITE_OMIT_LOAD_EXTENSION)
target_link_libraries(relive-backend PRIVATE ${SSL_BACKEND} ${SQLITE3_TARGET})

