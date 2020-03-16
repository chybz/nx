#ifndef __NX_SERVICE_H__
#define __NX_SERVICE_H__

#include <functional>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio.hpp>

#include <nx/config.h>
#include <nx/object_base.hpp>
#include <nx/handlers.hpp>
#include <nx/task.hpp>

namespace nx {

namespace asio = boost::asio;

class NX_API service
{
public:
    static service& get();

    service();
    ~service();

    void start();
    void stop();

    asio::io_context& io_context();
    const asio::io_context& io_context() const;

    void add(object_ptr sptr);
    void remove(object_ptr sptr);

    service& operator<<(void_cb&& cb);

    task_ptr available_task();
    void remove_task(task_ptr tptr);

private:
    service(const service&) = delete;
    void operator=(const service&) = delete;

    asio::io_context io_context_;
    asio::io_context::work work_;
    std::thread t_;

    std::unordered_set<object_ptr> objects_;
    std::mutex objects_mutex_;

    std::unordered_set<object_ptr> available_tasks_;
    std::unordered_set<object_ptr> runnable_tasks_;
    std::mutex tasks_mutex_;
};

NX_API
void
add_object(object_ptr ptr);

NX_API
void
remove_object(object_ptr ptr);

template <typename Derived, typename... Args>
std::shared_ptr<Derived>
new_object(Args&&... args)
{
    auto ptr = std::make_shared<Derived>(std::forward<Args>(args)...);

    add_object(ptr->ptr());

    return ptr;
}

NX_API
void
stop();

NX_API
service&
async();

} // namespace nx

#endif // __NX_SERVICE_H__
