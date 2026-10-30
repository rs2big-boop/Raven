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
bool tcp_poll_read(int fd, int timeout_ms = 100);
void tcp_close(int fd);

// UDP Datagram primitives
int udp_bind(const std::string& host, int port);
int udp_send_to(int fd, const std::string& host, int port, const std::string& data);
struct UdpPacket {
    std::string data;
    std::string sender_ip;
    int sender_port = 0;
};
UdpPacket udp_recv_from(int fd, size_t max_bytes = 65536);
void udp_close(int fd);

} // namespace raven::platform
