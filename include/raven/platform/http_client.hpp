#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace raven::platform {

struct HttpRequest {
    std::string method = "GET";
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    int timeout_ms = 10000;
    bool follow_redirects = true;
};

struct HttpResponse {
    int status = 0;
    std::string status_text;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool ok = false;
    std::string error;
};

HttpResponse http_request(const HttpRequest& req);

} // namespace raven::platform
