#pragma once
#include <sys/socket.h>
#include <netdb.h>

#include <string>
#include <string.h>
#include <memory>
#include <vector>
#include <iostream>

#define MAX_ATTEMPTS 20

class Client {
    int _socket;

public:
    //connects to server on host:port
    Client(char* host, char* port);

    //sends a message
    bool send_message(std::string msg);

    bool receive(std::string& result, const std::size_t buffer_size);

    //destructor -> closes socket
    ~Client();


};