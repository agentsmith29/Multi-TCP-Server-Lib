//
// Created by developer on 08.02.20.
//

#include "../include/ServerWorker.h"

// Files for Logging
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// import the sinks
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/daily_file_sink.h" // suprt for daily file logging

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

 #include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

using namespace mServer;
using std::exception;
using std::cout;
using std::endl;



int ServerWorker::createLogger(){

    _logger_name = "Svr Worker " + std::to_string(_worker_id);
    // Create the logger
    try {
        // Creating loggers with multiple sinks
        // see: https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>("/tmp/serverlog.txt", 23, 59));
        _logger = std::make_shared<spdlog::logger>(_logger_name, begin(sinks), end(sinks));
        //register it if you need to access it globally
        spdlog::register_logger(_logger);
        spdlog::set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    }
    catch(exception &ex){
        cout << ex.what() << endl;
    }

}

int ServerWorker::getNotificationDescriptor() {

    return _fd[0];
}

ServerWorker::ServerWorker(int worker_id, int socket_descriptor, struct sockaddr_in &address){

    _socket_descriptor = socket_descriptor;
    _worker_id = worker_id;

    createLogger();
    _logger->debug("Worker {0} for descriptor {1} ist warming up...", _worker_id, _socket_descriptor);

    pipe(_fd);

    _address = address;
    //clear the socket set
    FD_ZERO(&_readfds);
    //if valid socket descriptor then add to read list
    if(socket_descriptor > 0)
        FD_SET(socket_descriptor, &_readfds);
    else
        _logger->error("Descriptor was zero or less!");

    _done = false;
    // Starting the listener
    std::thread t(&ServerWorker::serve, this, std::ref(_done));//
    _logger->info("Worker {0} is ready!", _worker_id, _socket_descriptor);

    // Detach the thread
    t.detach();
}


bool ServerWorker::hasEnded() {

    // Print status.
    if (_done == true) {
        _logger->debug("Worker {0} has ended!", _worker_id);
    }
    return _done;
}
//
int ServerWorker::serve(std::atomic<bool> &done) {


    int64_t activity_on_descriptor = 0;

    int valread = 0;

    char buffer[4096];



    _logger->info("Worker {0} ist ready for listening.", _worker_id);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while(true) {
        //
        activity_on_descriptor = select( _socket_descriptor + 1, &_readfds ,
                NULL , NULL , NULL);

        if ((activity_on_descriptor < 0) && (errno!=EINTR)) {
            _logger->error("Select error: {0}. Code {1}.", strerror(errno), errno);
        }

        if (FD_ISSET(_socket_descriptor, &_readfds)) {
             _logger->debug("Event received!", _socket_descriptor);
            if ((valread = read(_socket_descriptor, buffer, 1024)) == 0) {
                // Somebody disconnected , get his details and print
                handleDisconnectClientRequest();
                _logger->debug("Received disconnect command on worker {0} (Descriptor is {1})",
                        _worker_id, _socket_descriptor);
                // Delete the logger, thread is about to end
                spdlog::drop(_logger_name);
                // Set the atomic value to true to get the thread's state
                done = true;
                return 1;
            }
            else {
                // Echo back the message that came in set the string terminating NULL byte
                // on the end of the data read
                buffer[valread] = '\0';
                _logger->debug("Received [ {0} ] from client with descriptor {1}", buffer, _socket_descriptor);
                //sendMessage(sd , buffer);
            }
        }



    }
#pragma clang diagnostic pop
   // p.set_value(0);
}

int ServerWorker::handleDisconnectClientRequest() {

    int addrlen = sizeof(_address);

    getpeername(_socket_descriptor, (struct sockaddr *) &_address, (socklen_t*) &addrlen);

    // inform user of socket number - used in send and receive commands
    _logger->info("Host {0}:{1} disconnected. Remove descriptor {2}." ,
                  inet_ntoa(_address.sin_addr),
                  ntohs(_address.sin_port),
                  _socket_descriptor);

    // Close the socket and mark as 0 in list for reuse
    close(_socket_descriptor);
    return 0;

}

ServerWorker::~ServerWorker() {
    write(_fd[1], "end", (strlen("end")+1));
    close(_fd[0]);
    close(_fd[1]);
    _logger->debug("Worker {0} destroyed!", _worker_id);
}
