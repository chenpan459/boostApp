#pragma once

#include "namespace.hpp"

#include <boost/asio/io_context.hpp>

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

NV_NS_INFRA_NET_BEGIN

class IoContextPool {
public:
    explicit IoContextPool(std::size_t thread_count);

    boost::asio::io_context& io_context();
    boost::asio::io_context& next_io_context();

    void run();
    void stop();

private:
    struct Entry {
        explicit Entry(std::size_t id);
        boost::asio::io_context io;
        using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
        WorkGuard work;
    };

    std::vector<std::unique_ptr<Entry>> entries_;
    std::vector<std::thread> threads_;
    std::size_t thread_count_{1};
};

NV_NS_INFRA_NET_END
