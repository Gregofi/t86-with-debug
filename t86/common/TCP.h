#pragma once

#include "messenger.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <span>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace TCP {

class TCPError : public std::exception {
public:
    TCPError(std::string message)
        : message(std::move(message)) { }

    const char* what() const noexcept override { return message.c_str(); }

private:
    std::string message;
};

class Batch {
public:
    Batch& AddMessage(std::string message) {
        data.emplace_back(std::move(message));
        return *this;
    }

    /// Moves the contents of the class out of
    /// this batch. The class shouldn't be used
    /// after this call.
    std::vector<std::string> YieldBatch() { return std::move(data); }

private:
    std::vector<std::string> data;
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
    SendRaw(socket, x.data, sizeof(size_t));
    SendRaw(socket, reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
}

inline std::optional<std::string> Receive(int socket) {
    // The messages are in form 8b | data
    size_t size = 0;
    int err = read(socket, &size, sizeof(size_t));
    if (err < 0) {
        throw TCPError("Reading failed");
    }
    if (err == 0) {
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
    // Unecessary copy, but since the messages will be very small we frankly
    // dont care
    std::string result(data.begin(), data.end());
    return result;
}

template <typename T> class TCP : public Messenger {
public:
    TCP(int port)
        : port(port) { }
    void Initialize() {
        if (initialized) {
            throw TCPError("Already initialized");
        }
        static_cast<T*>(this)->Initialize_();
        initialized = true;
    }

    void Send(const std::string& s) override {
        if (!initialized) {
            throw TCPError("Call to Send before Initialize");
        }
        ::TCP::Send(sock, s);
    }

    std::optional<std::string> Receive() override {
        if (!initialized) {
            throw TCPError("Call to Receive before Initialize");
        }
        return ::TCP::Receive(sock);
    }

protected:
    bool initialized = false;
    int port;
    int sock;
};

class TCPClient : public TCP<TCPClient> {
public:
    TCPClient(int port)
        : TCP(port) { }

    void Initialize_() {
        if (initialized) {
            throw TCPError("Already connected");
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw TCPError("Unable to open socket");
        }

        sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port),
        };

        if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            throw TCPError("Unable to connect");
        }
    }

    ~TCPClient() { close(sock); }
};

class TCPServer : public TCP<TCPServer> {
public:
    TCPServer(int port)
        : TCP(port) { }

    /**
     * Listens for incoming connections, blocking call
     */
    void Initialize_() {
        int opt = 1;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw TCPError("Couldn't open socket - socket");
        }

        if (setsockopt(
                server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            throw TCPError(std::string("Couldn't open socket - setsockopt:")
                + strerror(errno));
        }

        sockaddr_in address {
            .sin_family = AF_INET,
            .sin_port = htons(port),
        };
        address.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            throw TCPError("Couldn't bind socket to the port");
        }

        if (listen(server_fd, 3) < 0) {
            throw TCPError("Listen Error");
        }

        if (listen(server_fd, 3) < 0) {
            throw TCPError("Listen error");
        }

        int addrlen = sizeof(address);
        sock = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen);
    }

    ~TCPServer() {
        if (initialized) {
            shutdown(server_fd, SHUT_RDWR);
        }
    }

protected:
    int server_fd;
};

}
