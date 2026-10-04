#include "web_server.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

std::string queryParam(const std::string& path, const std::string& key) {
    std::size_t q = path.find('?');
    if (q == std::string::npos) return "";
    std::string needle = key + "=";
    std::size_t pos = path.find(needle, q);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    std::size_t end = path.find('&', pos);
    return path.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

static void handleClient(int client, const HttpHandler& handler) {
    // 1) read the request (we only need the first line: "GET /path HTTP/1.1")
    char buf[2048];
    ssize_t n = ::recv(client, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    std::string req(buf);
    HttpResponse res;
    if (req.compare(0, 4, "GET ") != 0) {
        res.status = 405; res.body = "only GET is supported";
    } else {
        std::size_t end = req.find(' ', 4);
        res = handler(req.substr(4, end - 4));
    }

    // 2) write the response: status line + headers + blank line + body
    const char* text = res.status == 200 ? "OK" : (res.status == 404 ? "Not Found" : "Error");
    char head[256];
    int hl = std::snprintf(head, sizeof(head),
        "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
        res.status, text, res.contentType.c_str(), res.body.size());
    ::send(client, head, hl, MSG_NOSIGNAL);
    ::send(client, res.body.data(), res.body.size(), MSG_NOSIGNAL);
}

bool runWebServer(const std::string& bindIp, int port, const HttpHandler& handler,
                  volatile std::sig_atomic_t& stop) {
    int server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) return false;

    int yes = 1;
    ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, bindIp.c_str(), &addr.sin_addr) != 1 ||
        ::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(server, 16) < 0) {
        ::close(server);
        return false;
    }

    while (!stop) {
        pollfd pfd{server, POLLIN, 0};
        if (::poll(&pfd, 1, 200) <= 0) continue;     // wake up every 200 ms to check `stop`
        int client = ::accept(server, nullptr, nullptr);
        if (client < 0) continue;
        handleClient(client, handler);
        ::close(client);
    }
    ::close(server);
    return true;
}
