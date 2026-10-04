#pragma once

#include <csignal>
#include <functional>
#include <string>

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain";
    std::string body;
};


using HttpHandler = std::function<HttpResponse(const std::string& path)>;


bool runWebServer(const std::string& bindIp, int port, const HttpHandler& handler,
                  volatile std::sig_atomic_t& stop);


std::string queryParam(const std::string& path, const std::string& key);
