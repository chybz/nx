#include <iostream>
#include <regex>

#include <nx/uri.hpp>
#include <nx/escape.hpp>
#include <nx/http_status.hpp>
#include <nx/regexp/uri.hpp>
#include <nx/utils.hpp>

namespace nx {

uri::uri(const std::string& u)
{ parse(u); }

std::string&
uri::scheme()
{ return scheme_; }

const std::string&
uri::scheme() const
{ return scheme_; }

std::string&
uri::host()
{ return host_; }

const std::string&
uri::host() const
{ return host_; }

uint16_t&
uri::port()
{ return port_; }

const uint16_t
uri::port() const
{ return port_; }

std::string&
uri::path()
{ return path_; }

const std::string&
uri::path() const
{ return path_; }

attributes&
uri::a()
{ return a_; }

const attributes&
uri::a() const
{ return a_; }

void
uri::parse(const std::string& u)
{
    std::smatch m;
    std::size_t pos = 0;

    // Sane defaults
    scheme_ = "http";
    host_ = "localhost";
    port_ = 80;

    if (match(u, regexp::nx_uri, m)) {
        // Full HTTP URI
        pos = 2;
        scheme_ = m[pos++].str();
        host_ = m[pos++].str();

        if (m[pos].matched) {
            port_ = to_num<uint16_t>(m[pos].str());
        }

        // Index of path
        pos = 7;
    } else if (match(u, regexp::nx_path, m)) {
        // Only HTTP path
        pos = 3;
    } else {
        throw BadRequest;
    }

    path_ = "/" + m[pos++].str();

    if (m[pos].matched) {
        // Query
        a_ << attributes(m[pos].str(), '&');
    }
}

} // namespace nx
