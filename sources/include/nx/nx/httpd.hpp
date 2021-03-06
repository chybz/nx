#ifndef __NX_HTTPD_H__
#define __NX_HTTPD_H__

#include <string>

#include <nx/config.h>
#include <nx/http.hpp>
#include <nx/methods.hpp>
#include <nx/route.hpp>

namespace nx {

class NX_API httpd
{
public:
    route& operator()(const method& m);
    route& operator()(const ws_tag& m);


    endpoint operator()(const endpoint& ep);
    endpoint_tcp operator()(const endpoint_tcp& ep);
    endpoint_local operator()(const endpoint_local& ep);

private:
    void operator()(request& req, buffer& data, reply& rep);

    http_tcp s_;
    http_local local_s_;
    routes_map routes_map_;
};

} // namespace nx

#endif // __NX_HTTPD_H__
