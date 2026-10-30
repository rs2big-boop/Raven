#include "raven/platform/http_client.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

namespace raven::platform {

namespace {

std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

} // namespace

HttpResponse http_request(const HttpRequest& req) {
    HttpResponse resp;

    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        resp.ok = false;
        resp.error = "Failed to create IPC pipes";
        return resp;
    }

    pid_t pid = fork();
    if (pid < 0) {
        resp.ok = false;
        resp.error = "Failed to fork process";
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return resp;
    }

    if (pid == 0) {
        // Child process
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        std::vector<std::string> args = {
            "curl",
            "-s",
            "-i",
            "-X", req.method,
            "-m", std::to_string(std::max(1, req.timeout_ms / 1000))
        };

        if (req.follow_redirects) {
            args.push_back("-L");
            args.push_back("--max-redirs");
            args.push_back("10");
        }

        for (const auto& [k, v] : req.headers) {
            args.push_back("-H");
            args.push_back(k + ": " + v);
        }

        if (!req.body.empty()) {
            args.push_back("--data-binary");
            args.push_back("@-"); // Read body from stdin directly, no /tmp/ files!
        }

        args.push_back(req.url);

        std::vector<char*> c_args;
        for (auto& s : args) {
            c_args.push_back(s.data());
        }
        c_args.push_back(nullptr);

        execvp("curl", c_args.data());
        _exit(127);
    }

    // Parent process
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Write body to stdin pipe if present
    if (!req.body.empty()) {
        const char* data = req.body.data();
        size_t remaining = req.body.size();
        while (remaining > 0) {
            ssize_t written = write(stdin_pipe[1], data, remaining);
            if (written <= 0) break;
            data += written;
            remaining -= static_cast<size_t>(written);
        }
    }
    close(stdin_pipe[1]); // EOF to curl stdin

    // Read response output
    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(bytes_read));
    }
    close(stdout_pipe[0]);

    std::string err_output;
    while ((bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer))) > 0) {
        err_output.append(buffer, static_cast<size_t>(bytes_read));
    }
    close(stderr_pipe[0]);

    int status_val = 0;
    waitpid(pid, &status_val, 0);

    if (output.empty()) {
        resp.ok = false;
        resp.status = 0;
        resp.error = err_output.empty() ? "Network error or connection refused" : err_output;
        return resp;
    }

    // Parse HTTP response format: headers followed by \r\n\r\n (or \n\n) then body
    // If redirects occurred, multiple HTTP header blocks can exist; locate the last HTTP/x.x line
    size_t last_http_pos = 0;
    size_t search_pos = 0;
    while (true) {
        size_t pos = output.find("HTTP/", search_pos);
        if (pos == std::string::npos) break;
        last_http_pos = pos;
        search_pos = pos + 5;
    }

    size_t header_end = output.find("\r\n\r\n", last_http_pos);
    size_t delim_len = 4;
    if (header_end == std::string::npos) {
        header_end = output.find("\n\n", last_http_pos);
        delim_len = 2;
    }

    std::string header_block;
    if (header_end != std::string::npos) {
        header_block = output.substr(last_http_pos, header_end - last_http_pos);
        resp.body = output.substr(header_end + delim_len);
    } else {
        header_block = output.substr(last_http_pos);
        resp.body = "";
    }

    std::istringstream hstream(header_block);
    std::string line;
    if (std::getline(hstream, line)) {
        line = trim_str(line);
        // e.g. "HTTP/1.1 200 OK"
        std::istringstream lstream(line);
        std::string proto;
        int status_code = 0;
        lstream >> proto >> status_code;
        resp.status = status_code;
        std::string text;
        std::getline(lstream, text);
        resp.status_text = trim_str(text);
    }

    while (std::getline(hstream, line)) {
        line = trim_str(line);
        if (line.empty()) continue;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim_str(line.substr(0, colon));
            std::string val = trim_str(line.substr(colon + 1));
            resp.headers[key] = val;
        }
    }

    resp.ok = (resp.status >= 200 && resp.status < 400);
    return resp;
}

} // namespace raven::platform
