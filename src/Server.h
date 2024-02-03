#pragma once 

#include <sys/socket.h>
#include <netdb.h>
#include <memory>

#define LISTEN_BACKLOG 20

class Server {

public:
    //starts the server (bind, listen, accept) returns socket
    static int startSever(char* port);
};