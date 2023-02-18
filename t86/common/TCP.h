#pragma once

#include <iostream>
#include <vector>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <exception>
#include <cstdint>
#include <cstring>
#include <span>
#include <optional>
#include <string>

namespace TCP {

class TCPError: public std::exception {
public:
    TCPError(std::string message): message(std::move(message)) {

    }

    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

/// C++20: Use std::span
inline void SendRaw(int socket, const uint8_t* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        sent += send(socket, &data[sent], size - sent, 0);
    }
}

inline void Send(int socket, const std::string& s) {
    union {
        size_t l;
        uint8_t data[sizeof(size_t)];
    } x;
    x.l = s.length();
    std::cerr << "size of the string: " << s.length() << "\n";
    std::cerr << "size of the string in the union: " << x.l << " " << x.data[0] << "\n";
    SendRaw(socket, x.data, sizeof(size_t));

    SendRaw(socket, reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
}

class Receiver {
public:
    Receiver() {}
    std::optional<std::string> Receive(int socket) {
        // The messages are in form 8b | data
        size_t size = 0;
        int err = read(socket, &size, sizeof(size_t));
        if (err < 0) {
            throw TCPError("Reading failed");
        } if (err == 0) {
            return std::nullopt;
        }

        size_t received = 0;
        std::vector<uint8_t> data(size);
        while (received < size) {
            int err = read(socket, data.data() + received, size - received);
            if (err < 0) {
                throw TCPError("Reading failed");
            } else if (err == 0) {
                return std::nullopt;
            }
            received += err;
        }
        // Unecessary copy, but since the messages will be very small we franky
        // dont care
        std::string result(data.begin(), data.end());
        return result;
    }
};

class TCPClient {
public:
    TCPClient(int port): port(port) {
        char buffer[1024];
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw TCPError("Unable to open socket");
        }

        sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port),
        };
        
        int client_fd = connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    }

    void Send(const std::string& s) {
        ::TCP::Send(sock, s);
    }

    std::optional<std::string> Receive() {
        return receiver.Receive(sock);
    }

    ~TCPClient() {
        close(sock);
    }
public:
    int port;
    int sock;
    Receiver receiver;
};

class TCPServer {
public:
    TCPServer(int port): port(port) {
        int opt = 1;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw TCPError("Couldn't open socket - socket");
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                       sizeof(opt))) {
            throw TCPError("Couldn't open socket - setsockopt");
        }

        sockaddr_in address{
            .sin_family = AF_INET,
            .sin_port = htons(port),
        };
        address.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0) {
            throw TCPError("Couldn't bind socket to the port");
        }

        if (listen(server_fd, 3) < 0) {
            throw TCPError("Listen Error");
        }

        int addrlen = sizeof(address);
        sock = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen);
    }

    ~TCPServer() {
        shutdown(server_fd, SHUT_RDWR);
    }

    void Send(const std::string& s) {
        ::TCP::Send(sock, s);
    }

    std::optional<std::string> Receive() {
        return receiver.Receive(sock);
    }
private:
    static const size_t BUFFER_SIZE = 4096;

    int port;
    int server_fd;
    int sock;

    uint8_t buffer[BUFFER_SIZE];
    uint8_t size = 0;

    Receiver receiver;
};

}
