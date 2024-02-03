#include "Client.h"



Client::Client(char* host, char* port) {
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;;


    if (int status = getaddrinfo(host, port, &hints, &res); status != 0) {
        freeaddrinfo(res);
        throw std::runtime_error("Getaddrinfo error: " + std::string(gai_strerror(status)));
    }

    _socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_socket == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("Socket error " + std::string(strerror(errno)));
    }

    int attempt = 0;
    while (attempt++ < MAX_ATTEMPTS && connect(_socket, res->ai_addr, res->ai_addrlen) == -1) {
        std::cerr << "Worker: failed attempt " << attempt << std::endl;
        close(_socket);
    }

    freeaddrinfo(res);

    if (_socket == -1)
        throw std::runtime_error("Impossible to connect to server");
}


bool Client::send_message(std::string msg) {
    if (send(_socket, msg.c_str(), msg.length(), 0) == -1) {
        perror("Worker: sent failed");
        return false;
    }

    return true;
}

bool Client::receive(std::string& result, const std::size_t buffer_size) {
    std::vector<char> buffer(buffer_size);
    int nbytes = (int)recv(_socket, buffer.data(), buffer.size(), 0);

    //error or socket closed
    if (nbytes <= 0)
        return false;

    //received valid data
    result = std::string(buffer.data(), nbytes);
    return true;
}

Client::~Client() {
    close(_socket);
}

