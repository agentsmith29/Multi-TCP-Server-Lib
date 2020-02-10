//
// Created by developer on 07.02.20.
//


#ifndef MULTI_TCP_SERVER_LIB_SERVER_H
#define MULTI_TCP_SERVER_LIB_SERVER_H

#include "spdlog/spdlog.h"
#include "ServerWorker.h"

#include <string>
#include <memory>
#include <list>
#include <queue>
#include <sys/socket.h>
#include <arpa/inet.h>


namespace mServer{

    class Server{

    public:
        Server(int port);

        // Initialize the master socket for accepting incoming requests
        int initMasterSocket();

        int startListening();

        int startListening_1();

        ~Server();
    private:

        // For logging any kind of messages
        std::shared_ptr<spdlog::logger> _logger;

        // Server's binding port
        int _binding_port = 0;

        // Servers master socketfd
        int _master_socket = 0;

        // Set the maximum client number
        int _maximum_clients = 1;

        int *_client_socket;

        //set of socket descriptors
        fd_set _readfds;

        int _max_sd = 0;



        // Socket address
        struct sockaddr_in _address;

        // Handles if the master receives an incoming request
        int acceptNewIncomingRequest();

        int sendMessage(int socket_descriptor, std::string message);

        int handleDisconnectClientRequest(int socket_descriptor, int client_socket_position);






    };
}
#endif //MULTI_TCP_SERVER_LIB_SERVER_H
