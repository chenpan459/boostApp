#include "test_assert.hpp"

int test_router();
int test_json_util();
int test_http_util();
int test_middleware();

int main() {
    int failures = 0;
    failures += test_router();
    failures += test_json_util();
    failures += test_http_util();
    failures += test_middleware();

    if (failures == 0) {
        return 0;
    }
    return 1;
}
