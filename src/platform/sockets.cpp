#include "raven/platform/sockets.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <vector>

namespace raven::platform {

int tcp_connect(const std::string& host, int port, int timeout_ms) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        return -1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return -1;
    }

    // Set non-blocking for connect timeout
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int ret = connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (ret < 0 && errno == EINPROGRESS) {
        struct pollfd pfd{};
        pfd.fd = sock;
        pfd.events = POLLOUT;
        int poll_res = poll(&pfd, 1, timeout_ms);
        if (poll_res <= 0) {
            close(sock);
            return -1;
        }
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error != 0) {
            close(sock);
            return -1;
        }
    } else if (ret < 0) {
        close(sock);
        return -1;
    }

    // Restore blocking
    fcntl(sock, F_SETFL, flags);
    return sock;
}

int tcp_listen(const std::string& host, int port, int backlog) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host.empty() || host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    }

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    if (listen(sock, backlog) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

int tcp_accept(int server_fd) {
    struct sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    return accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
}

int tcp_send(int fd, const std::string& data) {
    if (fd < 0) return -1;
    ssize_t sent = send(fd, data.data(), data.size(), 0);
    return static_cast<int>(sent);
}

std::string tcp_recv(int fd, size_t max_bytes) {
    if (fd < 0 || max_bytes == 0) return "";
    std::vector<char> buffer(max_bytes);
    ssize_t r = recv(fd, buffer.data(), buffer.size(), 0);
    if (r <= 0) return "";
    return std::string(buffer.data(), static_cast<size_t>(r));
}

bool tcp_poll_read(int fd, int timeout_ms) {
    if (fd < 0) return false;
    struct pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    int res = poll(&pfd, 1, timeout_ms);
    return (res > 0 && (pfd.revents & POLLIN));
}

void tcp_close(int fd) {
    if (fd >= 0) close(fd);
}

int udp_bind(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host.empty() || host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    }

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

int udp_send_to(int fd, const std::string& host, int port, const std::string& data) {
    if (fd < 0) return -1;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    ssize_t sent = sendto(fd, data.data(), data.size(), 0,
                          reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    return static_cast<int>(sent);
}

UdpPacket udp_recv_from(int fd, size_t max_bytes) {
    UdpPacket pkt;
    if (fd < 0 || max_bytes == 0) return pkt;

    std::vector<char> buffer(max_bytes);
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(from);

    ssize_t r = recvfrom(fd, buffer.data(), buffer.size(), 0,
                         reinterpret_cast<struct sockaddr*>(&from), &from_len);
    if (r > 0) {
        pkt.data.assign(buffer.data(), static_cast<size_t>(r));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
        pkt.sender_ip = ip;
        pkt.sender_port = ntohs(from.sin_port);
    }
    return pkt;
}

void udp_close(int fd) {
    if (fd >= 0) close(fd);
}

} // namespace raven::platform
