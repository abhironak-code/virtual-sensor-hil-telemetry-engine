#pragma once
// A tiny HTTP/1.0 server built directly on TCP sockets (socket/bind/listen/accept).
// It handles one request at a time, which is plenty for a local dashboard.
#include <csignal>
#include <functional>
#include <string>

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain";
    std::string body;
};

// handler(path) is called for every "GET <path>" request.
using HttpHandler = std::function<HttpResponse(const std::string& path)>;

// Runs until `stop` becomes non-zero. Returns false if the port cannot be opened.
bool runWebServer(const std::string& bindIp, int port, const HttpHandler& handler,
                  volatile std::sig_atomic_t& stop);

// Helper: get "value" from "/api/x?value=55&other=1"
std::string queryParam(const std::string& path, const std::string& key);
