#include "infra/net/io_context_pool.hpp"

#include "core/logger.hpp"

#include <thread>

NV_NS_INFRA_NET_BEGIN

IoContextPool::Entry::Entry(std::size_t id)
    : work(boost::asio::make_work_guard(io)) {
    (void)id;
}

IoContextPool::IoContextPool(std::size_t thread_count) : thread_count_(thread_count == 0 ? 1 : thread_count) {
    entries_.push_back(std::make_unique<Entry>(0));
    BOOSTAPP_LOG(Info, "io_context pool size=" + std::to_string(thread_count_));
}

boost::asio::io_context& IoContextPool::io_context() {
    return entries_.front()->io;
}

boost::asio::io_context& IoContextPool::next_io_context() {
    return entries_.front()->io;
}

void IoContextPool::run() {
    threads_.reserve(thread_count_);
    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this] {
            entries_.front()->io.run();
        });
    }

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void IoContextPool::stop() {
    for (auto& entry : entries_) {
        entry->work.reset();
        entry->io.stop();
    }
}

NV_NS_INFRA_NET_END
