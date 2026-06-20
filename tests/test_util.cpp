#include "core/json_util.hpp"
#include "core/http_util.hpp"
#include "test_assert.hpp"

int test_json_util() {
    int failures = 0;
    EXPECT_EQ(NV_NS_CORE::json_escape("hello"), "hello");
    EXPECT_EQ(NV_NS_CORE::json_escape(R"(a"b)"), R"(a\"b)");
    EXPECT_EQ(NV_NS_CORE::json_escape("line\nbreak"), R"(line\nbreak)");
    return failures;
}

int test_http_util() {
    int failures = 0;
    const auto [path, query] = NV_NS_CORE::split_path_query("/api/v1/packet?x=1");
    EXPECT_EQ(path, "/api/v1/packet");
    EXPECT_EQ(query, "x=1");

    const auto [path_only, empty_query] = NV_NS_CORE::split_path_query("/health");
    EXPECT_EQ(path_only, "/health");
    EXPECT_EQ(empty_query, "");
    return failures;
}
