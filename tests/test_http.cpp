#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "fuse/http.h"

namespace {

// Each test takes its own port so cases can run back to back without
// tripping over a lingering socket.
uint16_t next_port() {
    static std::atomic<uint16_t> p{39000};
    return p.fetch_add(1);
}

fuse_status Echo(const fuse_http_request *req, fuse_http_response *resp, void *) {
    if (std::strcmp(req->path, "/echo") == 0) {
        resp->status = 200;
        resp->headers = strdup("Content-Type: text/plain\r\n");
        resp->body = std::malloc(req->body_len ? req->body_len : 1);
        if (req->body_len) std::memcpy(resp->body, req->body, req->body_len);
        resp->body_len = req->body_len;
        return FUSE_OK;
    }
    resp->status = 404;
    resp->headers = strdup("");
    return FUSE_OK;
}

struct Server {
    uint16_t port;
    volatile int stop = 0;
    std::thread thread;
    fuse_config cfg;

    Server() {
        port = next_port();
        fuse_config_init(&cfg);
        cfg.bind_address = "127.0.0.1";
        cfg.port = port;
        thread = std::thread(fuse_http_serve, &cfg, Echo, nullptr, &stop);
        // fuse_http_serve binds synchronously inside fuse_listen() before
        // this thread does anything else, but there's no signal back to
        // the caller for "bound and ready" short of this, give it a beat.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ~Server() {
        stop = 1;
        thread.join();
    }
};

fuse_config ClientConfig(uint16_t port) {
    fuse_config cfg;
    fuse_config_init(&cfg);
    cfg.host = "127.0.0.1";
    cfg.port = port;
    return cfg;
}

}  // namespace

TEST(Http, GetHeaderFindsValueCaseInsensitively) {
    const char *headers = "Content-Type: text/plain\r\nX-Request-Id: abc123\r\n";
    size_t len = 0;
    const char *v = fuse_http_get_header(headers, "x-request-id", &len);
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(std::string(v, len), "abc123");
}

TEST(Http, GetHeaderReturnsNullForMissingName) {
    size_t len = 0;
    EXPECT_EQ(fuse_http_get_header("Content-Type: text/plain\r\n", "Authorization", &len),
              nullptr);
}

TEST(Http, RoundTripsRequestAndResponseWithHeadersAndBody) {
    Server server;
    fuse_config ccfg = ClientConfig(server.port);

    fuse_http_request req;
    req.method = "POST";
    req.path = "/echo";
    req.headers = "X-Test: hello-world\r\nContent-Type: text/plain\r\n";
    const char *payload = "the quick brown fox";
    req.body = payload;
    req.body_len = std::strlen(payload);

    fuse_http_response resp;
    ASSERT_EQ(fuse_http_fetch(&ccfg, &req, &resp, 5000), FUSE_OK);
    EXPECT_EQ(resp.status, 200);
    EXPECT_STREQ(resp.reason, "OK");
    ASSERT_EQ(resp.body_len, std::strlen(payload));
    EXPECT_EQ(std::memcmp(resp.body, payload, resp.body_len), 0);

    size_t ctlen = 0;
    const char *ct = fuse_http_get_header(resp.headers, "content-type", &ctlen);
    ASSERT_NE(ct, nullptr);
    EXPECT_EQ(std::string(ct, ctlen), "text/plain");

    fuse_http_response_free(&resp);
}

// The regression case: zero headers means the blank line is the first
// line's own \r\n immediately followed by one more \r\n, with nothing
// left over to pair the second \r\n with if the parser only looks after
// the first line (see http.cpp's ParseMessage comment).
TEST(Http, HandlesRequestAndResponseWithNoHeaders) {
    Server server;
    fuse_config ccfg = ClientConfig(server.port);

    fuse_http_request req;
    req.method = "GET";
    req.path = "/missing";
    req.headers = "";
    req.body = nullptr;
    req.body_len = 0;

    fuse_http_response resp;
    ASSERT_EQ(fuse_http_fetch(&ccfg, &req, &resp, 5000), FUSE_OK);
    EXPECT_EQ(resp.status, 404);
    EXPECT_STREQ(resp.reason, "Not Found");
    fuse_http_response_free(&resp);
}

TEST(Http, EachConnectionIsOneExchange) {
    Server server;
    fuse_config ccfg = ClientConfig(server.port);

    for (int i = 0; i < 3; ++i) {
        fuse_http_request req;
        req.method = "GET";
        req.path = "/echo";
        req.headers = "";
        req.body = nullptr;
        req.body_len = 0;

        fuse_http_response resp;
        ASSERT_EQ(fuse_http_fetch(&ccfg, &req, &resp, 5000), FUSE_OK) << "exchange " << i;
        EXPECT_EQ(resp.status, 200);
        fuse_http_response_free(&resp);
    }
}

TEST(Http, ResponseFreeIsIdempotentAndSafeOnZeroInit) {
    fuse_http_response resp;
    fuse_http_response_init(&resp);
    fuse_http_response_free(&resp);  // no-op, nothing was allocated
    fuse_http_response_free(&resp);  // safe to call twice
}
