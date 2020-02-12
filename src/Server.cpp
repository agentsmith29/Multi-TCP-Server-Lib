//
// Created by developer on 07.02.20.
//

// Files for Logging
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// import the sinks
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/daily_file_sink.h" // suprt for daily file logging

#include "../include/ServerExceptions.h"
#include "../include/Server.h"

#include <iostream>
#include <string>
#include <memory>
#include <list>

// for multiple threads
#include <thread>
#include "../include/Utils.h"

class list;

using namespace mServer;
using std::string;
using std::cout;
using std::endl;

#define workerholder std::list<std::pair<std::unique_ptr<ServerWorker>, int>>
workerholder _server_workers; // point
// er/id

Server::Server(int port){

    // Create the logger
    try {
        // Creating loggers with multiple sinks
        // see: https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>("/tmp/serverlog.txt", 23, 59));
        _logger = std::make_shared<spdlog::logger>("TCP Server Master", begin(sinks), end(sinks));
        //register it if you need to access it globally
        spdlog::register_logger(_logger);
        spdlog::set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    }
    catch(exception &ex){
        cout << ex.what() << endl;
    }



    _logger->info("Starting server instance.");

    _client_socket = new int[_maximum_clients];
    if(_client_socket == nullptr){
        _logger->error("Client Socket array o allocated! Aborting server initialization.", port);
        throw std::bad_alloc();
    }

    if (port < 1 || port > 65535) {
        _logger->error("Port {0} not allowed! Aborting server initialization.", port);
        throw ErrPortNotValid;
    }
    else {
        _binding_port = port;
    }


    _logger->info("Opening communication socket on port {0}.", _binding_port);
    initMasterSocket();

    if (_master_socket <= 0) {
        _logger->error("TCP Server not initialized properly. Exiting.", port);
        throw ErrSvrNotInit;
    }

    startListening();
    // startListening_1();



}

int Server::initMasterSocket(){

    int opt = true;

    if (_binding_port < 1 || _binding_port > 65535) {
        _logger->error("Port {0} not allowed! Aborting server initialization.", _binding_port);
        return -1;
    }
    // type of socket created
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_binding_port);



    //create a master socket
    _master_socket = socket(_address.sin_family, SOCK_STREAM, 0);



    if (_master_socket == 0) {
        _logger->error("Socket creation failed: {0}. Code {1}.", strerror(errno), errno);
        return -2;
    }
    else {
        _logger->debug("Allocating socket descriptor {0}: type {1}, protocol 0.", _master_socket, SOCK_STREAM);
    }


    // set master socket to allow multiple connections ,
    // this is just a good habit, it will work without this
    if(setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        _logger->error("Set socket options failed: {0}. Code {1}.", strerror(errno), errno);
        return -3;
    }
    else {
        _logger->debug("Set socket option: level {0}, optname {1}.",
                       SOL_SOCKET, SO_REUSEADDR);
    }


    // bind the socket to localhost and given port
    if (bind(_master_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        _logger->error("Socket binding failed: {0}. Code {1}.", strerror(errno), errno);
        return -4;
    }
    else {
        _logger->debug("Listener on Port {0}.", _binding_port);
    }

    //Specify maximum of 5 pending connections for the master socket
    if (listen(_master_socket, _maximum_clients) < 0) {
        _logger->error("Error listen: {0}. Code {1}.", strerror(errno), errno);
    }

    //clear the socket set
    FD_ZERO(&_readfds);

    //add master socket to set
    FD_SET(_master_socket, &_readfds);
    //max_sd = _master_socket;
    if(_master_socket > _max_sd)
        _max_sd = _master_socket;


    _logger->info("Initialized, ready for incoming connections.");

    return _master_socket;
}


int Server::startListening() {

    //set of socket descriptors
    int activity_on_descriptor = 0, valread = 0;

    char buffer[2048];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while(true) {
              // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity_on_descriptor = select( _max_sd + 1 , &_readfds , NULL , NULL , NULL);

        if ((activity_on_descriptor < 0) && (errno!=EINTR)) {
            _logger->error("Select error: {0}. Code {1}.", strerror(errno), errno);
        }
        else{
            _logger->debug("Event received on socket {0}", activity_on_descriptor);
        }
        // If something happened on the master socket ,
        // then its an incoming connection
        if (FD_ISSET(_master_socket, &_readfds)) {
            int new_socket = acceptNewIncomingRequest();
            // A new Request has been made, spwan a Server Receiver
            if(new_socket > 0) {

                int worker_id = _server_workers.size()+1;
                auto requestHandler = std::make_unique<ServerWorker>(worker_id, new_socket, _address);

                FD_SET(requestHandler->getNotificationDescriptor(), &_readfds);
                _logger->error("Added Pipe: {0}", requestHandler->getNotificationDescriptor());
                if(requestHandler->getNotificationDescriptor() > _max_sd)
                    _max_sd = requestHandler->getNotificationDescriptor();
                
                _server_workers.push_back(std::make_pair(std::move(requestHandler), worker_id) );

                _logger->error("Registered socket descriptor {0}", new_socket);
            }

        }

        _logger->debug("Running Services: {0}", _server_workers.size());

        for (auto& server_worker : _server_workers) {
                if (server_worker.first->hasEnded())
                    _server_workers.remove(server_worker);
                    break;
        }




       /* else {
            for (int i = 0; i < _maximum_clients; i++) {
                sd = _client_socket[i];
                if (FD_ISSET(sd, &readfds)) {
                    if ((valread = read(sd, buffer, 1024)) == 0) {
                        // Somebody disconnected , get his details and print
                        _logger->debug("Received disconnect command from client with descriptor {0}", sd);
                        handleDisconnectClientRequest(sd, i);
                    } else{
                        // Echo back the message that came in set the string terminating NULL byte
                        // on the end of the data read
                        buffer[valread] = '\0';
                        _logger->debug("Received {0} from client with descriptor {1}", buffer, sd);
                        //sendMessage(sd , buffer);
                    }
                }
            }

        } */

        // else its some IO operation on some other socket

    }
#pragma clang diagnostic pop
    return 0;
}

int Server::acceptNewIncomingRequest(){

    int new_socket = 0;
    int addrlen = sizeof(_address);

    new_socket = accept(_master_socket, (struct sockaddr *)&_address, (socklen_t*)&addrlen);

    if (new_socket < 0) {
        _logger->error("Accepting new client error: {0}. Code {1}.", strerror(errno), errno);
        return -1;
    }

    // inform user of socket number - used in send and receive commands
    _logger->info("New incoming connection from IP {0}, on port {1}. Socket descriptor is {2} , " ,
            inet_ntoa(_address.sin_addr),
            ntohs(_address.sin_port),
            new_socket);

    // Send message
    std::string message = "ECHO TCP Server v0.1 \r\n";
    if(sendMessage(new_socket, message) < 0)
        _logger->error("Can't send message to client {0}:{1}",
                       inet_ntoa(_address.sin_addr), ntohs(_address.sin_port));

    return new_socket;
/*
    //add new socket to array of sockets
    for (int i = 0; i < _maximum_clients; i++)
    {
        //if position is empty
        if( _client_socket[i] == 0 )
        {
            _client_socket[i] = new_socket;
            break;
        }
    }
*/
}

int Server::handleDisconnectClientRequest(int socket_descriptor, int client_socket_position) {

    int addrlen = sizeof(_address);

    getpeername(socket_descriptor, (struct sockaddr *) &_address, (socklen_t*) &addrlen);

    // inform user of socket number - used in send and receive commands
    _logger->info("Host {0}:{1} disconnected. Remove descriptor {2} on position {3} , " ,
                  inet_ntoa(_address.sin_addr),
                  ntohs(_address.sin_port),
                  socket_descriptor,
                  client_socket_position);

    //Close the socket and mark as 0 in list for reuse
    close(socket_descriptor);
    // Set the socket at the given position to zero
    _client_socket[client_socket_position] = 0;

}




int Server::sendMessage(int socket_descriptor, std::string message){
    //send new connection greeting message
    _logger->debug("Sending {0} to client with descriptor {1}", trim(message), socket_descriptor);
    if(send(socket_descriptor, message.c_str(), message.length(), 0) != message.length() )
    {
        _logger->error("Sending message failed: {0}. Code {1}.", strerror(errno), errno);
        return -1;
    }
    return 0;
}





Server::~Server(){
    _logger->info("Exit received. Closing all server instances.");
}
