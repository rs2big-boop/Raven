#pragma once
#include <string>
#include <cstdint>

namespace raven::platform {

// Native TCP socket primitives for low-level networking
int tcp_connect(const std::string& host, int port, int timeout_ms = 5000);
int tcp_listen(const std::string& host, int port, int backlog = 128);
int tcp_accept(int server_fd);
int tcp_send(int fd, const std::string& data);
std::string tcp_recv(int fd, size_t max_bytes = 65536);
void tcp_close(int fd);

} // namespace raven::platform
