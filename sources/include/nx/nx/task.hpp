#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio.hpp>

#include <nx/config.h>
#include <nx/object_base.hpp>
#include <nx/handlers.hpp>

namespace nx {

namespace asio = boost::asio;

class NX_API task
: public object_base
{
public:
    task()
    : io_context_(),
      work_(io_context_),
      t_([this](){
          io_context_.run();
          io_context_.reset();
      })
    {}

    virtual ~task()
    {}

    task(const task& ) = delete;
    task& operator= (const task& ) = delete;

    asio::io_context& get_io_context()
    { return io_context_; }

    const asio::io_context& get_io_context() const
    { return io_context_; }

    virtual void stop() override
    {
        io_context_.stop();
        t_.join();
        io_context_.reset();
    }

private:
    asio::io_context io_context_;
    asio::io_context::work work_;
    std::thread t_;
};

using task_ptr = std::shared_ptr<task>;

}   // namespace nx
