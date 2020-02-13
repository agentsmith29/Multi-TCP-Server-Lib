//
// Created by developer on 07.02.20.
//
#include "../include/Server.h"
#include "../include/ServerWorker.h"
#include "../include/ServerMessage.h"
// Files for Logging
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// import the sinks
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/daily_file_sink.h" // suprt for daily file logging

#include "../include/ServerExceptions.hpp"
#include "../include/Server.h"

#include <iostream>
#include <string>
#include <memory>
#include <list>
// for multiple threads
#include <thread>
#include "../include/Utils.h"

#include "include/global_config.h"

class list;

using namespace mServer;
using std::string;
using std::cout;
using std::endl;

#define workerholder std::list<std::pair<std::unique_ptr<ServerWorker>, int>>
workerholder _server_worker_registry; // point
// er/id

int Server::createMessage(){

    try {
        // Creating loggers with multiple sinks
        // see: https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>("/tmp/serverlog.txt", 23, 59));
        _logger = std::make_shared<spdlog::logger>("TCP Server Master", begin(sinks), end(sinks));
        //register it if you need to access it globally
        spdlog::register_logger(_logger);
        _logger->set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    }
    catch(exception &ex){
        cout << "Logger creation Error: " << ex.what() << endl;
        return -1;
    }
    return 0;
}

Server::Server(int port){

    lock_message_access.lock();

    // Create the logger
    createMessage();

    _logger->info("Starting server instance. Version {0}.{1}.{2}",
            PROJECT_VER_MAJOR,
            PROJECT_VER_MINOR,
            PROJECT_VER_PATCH);

    _client_socket = new int[_maximum_clients];

    if(_client_socket == nullptr){
        _logger->error("Client Socket array failed allocating! Aborting server initialization.", port);
        throw std::bad_alloc();
    }

    if (port < 1 || port > 65535) {
        _logger->error("Port {0} not allowed! Aborting server initialization.", port);
        throw PortNotValidException;
    }
    else {
        _binding_port = port;
    }


    _logger->info("Opening communication socket on port {0}.", _binding_port);
    _master_socket = initMasterSocket();

    if(_master_socket <= 0){
        _logger->error("Not initialized properly. Master socket file descriptor was {0}. Exiting.", _master_socket);
        throw ServerNotInitException;
    }

    // Starting the listener
    std::thread t(&Server::startListening, this);

    // Detach the thread
    t.detach();
}

int Server::initMasterSocket(){

    int master_socket = 0;

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
    master_socket = socket(_address.sin_family, SOCK_STREAM, 0);
    if (master_socket == 0) {
        _logger->error("Master socket creation failed: {0}. Code {1}.", strerror(errno), errno);
    }
    else {
        _logger->debug("Allocating socket descriptor {0}: type {1}, protocol 0.", _master_socket, SOCK_STREAM);
    }

    // set master socket to allow multiple connections ,
    // this is just a good habit, it will work without this
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        _logger->error("Set socket options failed: {0}. Code {1}.", strerror(errno), errno);
        return -3;
    }
    else {
        _logger->debug("Set socket option: level {0}, optname {1}.",
                       SOL_SOCKET, SO_REUSEADDR);
    }


    // bind the socket to localhost and given port
    if (bind(master_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        _logger->error("Socket binding failed: {0}. Code {1}.", strerror(errno), errno);
        return -4;
    }
    else {
        _logger->info("Socket bound, listening on Port {0}.", _binding_port);
    }

    // Specify maximum of 5 pending connections for the master socket
    if (listen(master_socket, _maximum_clients) < 0) {
        _logger->error("Error listen: {0}. Code {1}.", strerror(errno), errno);
    }

    //clear the socket set
    FD_ZERO(&_readfds);

    //add master socket to set
    FD_SET(master_socket, &_readfds);
    //max_sd = _master_socket;
    if(master_socket > _max_sd)
        _max_sd = master_socket;


    _logger->info("Initialized. Ready for incoming connections.");

    return master_socket;
}


int Server::startListening() {

    //set of socket descriptors
    int activity_on_descriptor = 0, valread = 0;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

    while(true) {
        // wait for an activity on one of the sockets , timeout is NULL ,so wait indefinitely
        activity_on_descriptor = select( _max_sd + 1 , &_readfds ,
                NULL , NULL , NULL);

        if ((activity_on_descriptor < 0) && (errno!=EINTR)) {
            _logger->error("Select error: {0}. Code {1}.", strerror(errno), errno);
            return -1;
        }
        // If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(_master_socket, &_readfds)) {
            _logger->debug("Event on Master ({0}) received: Processing incoming connection...", activity_on_descriptor);
            int new_socket = acceptNewIncomingRequest();
            // A new Request has been made, spwan a Server Worker
            if(new_socket > 0) {
                registerWorkerThread(new_socket);
            }
        }
        else {
            updateWorkerRegistry();
        }
        _logger->debug("Running Services: {0}", _server_worker_registry.size());
        if( _server_worker_registry.empty())
            exit(0);

    }
#pragma clang diagnostic pop
    return 0;
}


int Server::registerWorkerThread(int socket_descriptor) {

    // TODO: Substitute with generated ID
    int worker_id = _server_worker_registry.size() + 1;

    // Create a new Pointer to a ServerWorker
    auto requestHandler = std::make_unique<ServerWorker>(this, worker_id, socket_descriptor);

    //

    _server_worker_registry.push_back(std::make_pair(std::move(requestHandler), worker_id) );
    _logger->info("Registered socket descriptor {0} for worker {1}", socket_descriptor, worker_id);

    //clear the socket set and again add the workers stored in the registry
    refreshDescriptorFileset();






    return 0;
}

int Server::updateWorkerRegistry() {
    int valread;
    char buffer[1024];

    workerholder::iterator server_worker = _server_worker_registry.begin();
    while (server_worker != _server_worker_registry.end()) {

        if (FD_ISSET((*server_worker).first->notification_fds(), &_readfds)) {
            valread = read((*server_worker).first->notification_fds(), buffer, 1024);
            _logger->debug("Worker {0} notified: {1}", (*server_worker).first->worker_id(), buffer);
            buffer[0] = '\0';
        }

        bool hasEnded = (*server_worker).first->hasEnded();
        int notification_fds = (*server_worker).first->notification_fds();
        int worker_id = (*server_worker).first->worker_id();

        if (hasEnded) {
            _logger->error("Deregister worker for socket {0}", worker_id);
            //clear the socket set and again add the workers stored in the registry
            _server_worker_registry.erase(server_worker++);
        }
        else
            ++server_worker;
    }

    refreshDescriptorFileset();
    return 0;
}

int Server::refreshDescriptorFileset(){

    _logger->debug("Refreshing registry.");
    //clear the socket set
    FD_ZERO(&_readfds);

    // Reregister the master socket
    FD_SET(_master_socket, &_readfds);
        _max_sd = _master_socket;

    // Reregister previous opened sockets
    for (auto& server_worker : _server_worker_registry) {

        bool hasEnded = server_worker.first->hasEnded();
        int notification_fd = server_worker.first->notification_fds();
        int worker_id = server_worker.first->worker_id();

        if (!hasEnded) {
            FD_SET(notification_fd, &_readfds);

            if(notification_fd > _max_sd)
                _max_sd = notification_fd;

            _logger->debug("Added worker {0} with file set {1} to registry.", worker_id, notification_fd);
        }
        else {
            _logger->info("Worker {0} seems to be stopped! Omitting.", worker_id);
        }
    }

    return 0;
};
int Server::acceptNewIncomingRequest(){

    int new_socket = 0;
    int addrlen = sizeof(_address);

    new_socket = accept(_master_socket, (struct sockaddr *)&_address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        _logger->error("Accepting new client error: {0}. Code {1}.", strerror(errno), errno);
        return -1;
    }

    // inform user of socket number - used in send and receive commands
    _logger->info("New incoming connection from IP {0}, on port {1}. Socket descriptor is {2} " ,
            inet_ntoa(_address.sin_addr),
            ntohs(_address.sin_port),
            new_socket);

    // Send message if it ist not zero
    if (std::strcmp(_welcome_message.c_str(), "")) {
        if (sendMessage(new_socket, _welcome_message) < 0)
            _logger->error("Can't send message to client {0}:{1}",
                           inet_ntoa(_address.sin_addr), ntohs(_address.sin_port));
    }

    return new_socket;

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
    return 0;
}




int Server::sendMessage(int socket_descriptor, std::string message){
    //send new connection greeting message
    _logger->debug("Sending message <{0}> to client with descriptor {1}",
            trim(message), socket_descriptor);
    if(send(socket_descriptor, message.c_str(), message.length(), 0) != message.length() ) {
        _logger->error("Sending message failed: {0}. Code {1}.", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int Server::message_push(std::shared_ptr<ServerMessage> msg) {
    lock_message_access.unlock();
    std::unique_lock<std::mutex> lock(_lock_message_registry);

    _logger->info("New message {0}-{1} ready for consumption.", msg->timestamp_str(), msg->messageID_str());
    _message_registry.push(std::move(msg));
    cond_message_access.notify_one();
    return 0;
}

std::shared_ptr<ServerMessage> Server::message_front() {
    std::unique_lock<std::mutex> lock(_lock_message_registry);

    if(_message_registry.empty())
        throw MsgRegistryEmtpyException;
    auto res =_message_registry.front(); // **************
    return res;
}

void Server::message_pop() {
    std::unique_lock<std::mutex> lock(_lock_message_registry  );
    _message_registry.pop(); // **************
    if(_message_registry.empty())
        lock_message_access.lock();
}


void Server::waitForMessage(){
    lock_message_access.lock();
    _logger->debug("Consumed message. Messages in buffer left: {0}", _message_registry.size());
    lock_message_access.unlock();
}



Server::~Server(){
    _logger->info("Exit received. Closing all server instances.");
}
