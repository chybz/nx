#define BOOST_TEST_MODULE nester

#include <iostream>

#include <nx/unit_test.hpp>

#include <nx/nx.hpp>
#include <nx/utils.hpp>

/*
 * Nested http client request used to create the
 * reply of an httpd.
 *
 * hc -> hd -> hc2 -> hd2 -> r2 -> r
 */

BOOST_AUTO_TEST_CASE(nested)
{
    using namespace nx;
    using namespace std;

    nx::timer deadline;
    nx::cond_var cv;

    deadline(5) = [&](nx::timer& t) {
        t.stop();
        cv.notify();
    };

    deadline.start();

    /////////////// start httpd 2 /////////////////////
    const char* hello_world2 = "Hello, world 2!";
    bool got_request2 = false;

    httpd hd2;

    hd2(GET) / "hello2" = [&](const request& req, buffer& data, reply& rep) {
        got_request2 = true;
        rep
            << text_plain
            << hello_world2
            ;
    };

    auto ep2 = make_endpoint("127.0.0.1", 8082);
    auto sep2 = hd2(ep2);

    /////////////// start httpd 1 //////////////////////

    auto ep = make_endpoint("127.0.0.1", 8081);
    const char* hello_world = "Hello, world 1!";
    bool reply_ok = false, reply_ok2 = false;
    bool got_reply = false, got_reply2 = false;
    bool got_request = false;

    httpd hd;
    hd(GET) / "hello1" = [&](const request& req, buffer& data, reply& rep) {
        got_request = true;

        /////////////// send a HTTP GET to hd2 /////////////////
        httpc hc2;
        hc2(GET, sep2) / "hello2" = [&](const reply& rep2, buffer& data2) {
            got_reply2 = true;

            reply_ok2 = rep2 && data2 == hello_world2;

            rep
                << text_plain
                << data2 << hello_world
            ;

            deadline.stop();
            cv.notify();
        };
    };

    auto sep = hd(ep);

    ////////////////  start httpc /////////////////////

    httpc hc;
    hc(GET, sep) / "hello1" = [&](const reply& rep, buffer& data) {
        got_reply = true;

        reply_ok = rep && data == string(hello_world) + string(hello_world2);

        deadline.stop();
        cv.notify();
    };

    cv.wait();
    nx::stop();

    cout << "got_request:" << got_request << endl;
    cout << "got_reply:"<< got_reply << endl;
    cout << "reply_ok :"<< reply_ok << endl;

    cout << "got_request2:" << got_request2 << endl;
    cout << "got_reply2:"<< got_reply2 << endl;
    cout << "reply_ok2 :"<< reply_ok2 << endl;

    BOOST_CHECK_MESSAGE(got_request, "httpd did not get the request");
    BOOST_CHECK_MESSAGE(got_reply, "httpc did not get the reply");
    BOOST_CHECK_MESSAGE(reply_ok, "httpc did not get correct reply");

    BOOST_CHECK_MESSAGE(got_request2, "httpd2 did not get a request");
    BOOST_CHECK_MESSAGE(got_reply2, "httpc2 did not get a reply");
    BOOST_CHECK_MESSAGE(reply_ok2, "httpc2 did not get correct reply");

}
