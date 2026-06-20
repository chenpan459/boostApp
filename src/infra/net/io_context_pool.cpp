#include "infra/net/io_context_pool.hpp"

#include "core/logger.hpp"

#include <thread>

NV_NS_INFRA_NET_BEGIN

IoContextPool::Entry::Entry(std::size_t id)
    : work(boost::asio::make_work_guard(io)) {
    (void)id;
}

IoContextPool::IoContextPool(std::size_t thread_count) {
    if (thread_count == 0) {
        thread_count = 1;
    }

    entries_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        entries_.push_back(std::make_unique<Entry>(i));
    }

    BOOSTAPP_LOG(Info, "io_context pool size=" + std::to_string(thread_count));
}

boost::asio::io_context& IoContextPool::io_context() {
    return entries_.front()->io;
}

boost::asio::io_context& IoContextPool::next_io_context() {
    const auto index = round_robin_.fetch_add(1, std::memory_order_relaxed) % entries_.size();
    return entries_[index]->io;
}

void IoContextPool::run() {
    threads_.reserve(entries_.size());
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        threads_.emplace_back([this, i] {
            entries_[i]->io.run();
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
