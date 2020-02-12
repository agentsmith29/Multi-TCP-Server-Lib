//
// Created by developer on 08.02.20.
//

#include "../include/ServerWorker.h"
#include "../include/Notifications.h"

// Files for Logging
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// import the sinks
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/daily_file_sink.h" // support for daily file logging



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






ServerWorker::ServerWorker(int worker_id, int socket_fd, struct sockaddr_in &address){

    // Assigning
    _socket_fd = socket_fd;
    _worker_id = worker_id;
    _address = address;
    _done = false;

    // Initialize the worker
    int result = initWorker();

    // check the initilazation return value
    if(result == -1) {
        _done = true;
        this->~ServerWorker();
    }
    else if (result == -2) {
        notifiyMaster(NTFY_INIT_SVR_ABRT);
        _done = true;
        this->~ServerWorker();
    }

}

int ServerWorker::createLogger(){

    _logger_name = "Server Worker " + std::to_string(_worker_id);
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
    return 0;
}

int ServerWorker::initWorker(){

    createLogger();
    _logger->debug("Worker {0} for file descriptor {1} ist warming up...", _worker_id, _socket_fd);

    // Open a pipe for notifying the master of incoming events
    _logger->debug("Opening pipe for notifying master", _worker_id, _socket_fd);
    int result = pipe(_notification_fd);
    if (result < 0) {
        _logger->error("Error opening notification pipe for master: {0}. Code {1}. Aborting.",
                       strerror(errno), errno);
        return -1;
    }

    _logger->debug("Register file descriptor {0} to monitor the file.", _socket_fd, _socket_fd);
    //clear the socket set
    FD_ZERO(&_readfds);
    //if valid socket descriptor then add to read list
    if (_socket_fd > 0) {
        FD_SET(_socket_fd, &_readfds);
    }
    else {
        _logger->error("Descriptor was zero or less ({0})!", _socket_fd);
        return -2;
    }


    // Starting the listener
    std::thread t(&ServerWorker::serve, this, std::ref(_done));

    // Detach the thread
    t.detach();
    _logger->info("Worker {0} has started. Ready for incoming connections.", _worker_id);

    return 0;
}

bool ServerWorker::hasEnded() {

    // Print status.
    if (_done == true) {
        _logger->debug("Worker {0} has stopped.", _worker_id);
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
        activity_on_descriptor = select(_socket_fd + 1, &_readfds,  NULL , NULL , NULL);

        if ((activity_on_descriptor < 0) && (errno!=EINTR)) {
            _logger->error("Select error: {0}. Code {1}.", strerror(errno), errno);
        }

        if (FD_ISSET(_socket_fd, &_readfds)) {
            if ((valread = read(_socket_fd, buffer, 1024)) == 0) {
                // Somebody disconnected , get his details and print
                handleDisconnectClientRequest();
                done = true;
                return 1;
            }
            else {
                // Echo back the message that came in set the string terminating NULL byte
                // on the end of the data read
                buffer[valread] = '\0';
                _logger->debug("Received [ {0} ] from client with descriptor {1}", buffer, _socket_fd);
                //sendMessage(sd , buffer);
            }
        }



    }
#pragma clang diagnostic pop
   // p.set_value(0);
}

int ServerWorker::handleDisconnectClientRequest() {

    _lock_disconnect.lock();

    int addrlen = sizeof(_address);

    getpeername(_socket_fd, (struct sockaddr *) &_address, (socklen_t*) &addrlen);

    // inform user of socket number - used in send and receive commands
    _logger->info("Host {0}:{1} disconnected. Remove descriptor {2}." ,
                  inet_ntoa(_address.sin_addr),
                  ntohs(_address.sin_port),
                  _socket_fd);

    // Close the socket and mark as 0 in list for reuse
    close(_socket_fd);

    _logger->debug("Received disconnect command on worker {0}/descriptor {1}. Shutting down...",
                   _worker_id, _socket_fd);
    // Delete the logger, thread is about to end
    spdlog::drop(_logger_name);


    // Set the atomic value to true to get the thread's state

    notifiyMaster(NTFY_WKR_DISCON_RCV);
    _lock_disconnect.unlock();

    return 0;

}

int ServerWorker::notifiyMaster(std::string message){

    _lock_notify.lock();
    write(_notification_fd[1], message.c_str(), (strlen(message.c_str()) + 1));
    _lock_notify.unlock();
    return 0;
}





ServerWorker::~ServerWorker() {


    _logger->debug("Worker {0} terminated. I'll be back.", _worker_id);
    close(_notification_fd[1]);
    close(_notification_fd[0]);

}
