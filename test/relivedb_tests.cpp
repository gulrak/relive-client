#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "helper.hpp"
#include <backend/relivedb.hpp>
#include <backend/system.hpp>

using namespace std::string_literals;

TEST_CASE("ReLiveDB config test", "[relivedb]")
{
    TemporaryDirectory t;
    relive::dataPath(t.path());
    relive::ReLiveDB rdb;
    // first check that we don't work on a real config database
    REQUIRE(rdb.getConfigValue(relive::Keys::relive_root_server, "---"s) == "---");
    CHECK(rdb.getConfigValue("not-set", "not set"s) == "not set");
    CHECK_NOTHROW(rdb.setConfigValue("some-string", "test"));
    CHECK(rdb.getConfigValue("some-string", "not set"s) == "test");
    CHECK_NOTHROW(rdb.setConfigValue("some-int", 1234));
    CHECK(rdb.getConfigValue("some-int", 42) == 1234);
}

