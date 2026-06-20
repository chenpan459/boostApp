#include "core/application.hpp"

int main(int argc, char** argv) {
    NV_NS_CORE::Application app(argc, argv, "nvcommd", true);
    return app.run_network();
}
