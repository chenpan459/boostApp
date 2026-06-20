#include "platform/cpu_affinity.hpp"

#include "core/logger.hpp"

#include <pthread.h>
#include <sched.h>

NV_NS_PLATFORM_BEGIN

bool bind_current_thread_to_cpu(int cpu) {
    if (cpu < 0) {
        return true;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(static_cast<std::size_t>(cpu), &cpuset);

    const int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    if (rc != 0) {
        BOOSTAPP_LOG(Warning, "failed to bind thread to cpu " + std::to_string(cpu));
        return false;
    }

    BOOSTAPP_LOG(Info, "thread bound to cpu " + std::to_string(cpu));
    return true;
}

NV_NS_PLATFORM_END
