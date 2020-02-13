//
// Created by developer on 08.02.20.
//

#include "../include/ServerWorker.h"
#include "../include/Notifications.h"
#include "../include/ServerMessage.h"

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




ServerWorker::ServerWorker(Server *master_svr, int worker_id, int socket_fd) {
    _master_svr = master_svr;

    // Assigning
    _socket_fd = socket_fd;
    _worker_id = worker_id;
    _address = *_master_svr->address();
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

/*
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
*/
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
        _logger->set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    }
    catch(exception &ex){
        cout << ex.what() << endl;
    }
    return 0;
}

int ServerWorker::initWorker(){

    createLogger();
    _logger->info("Worker {0} for file descriptor {1} ist starting up...", _worker_id, _socket_fd);

    // Open a pipe for notifying the master of incoming events
    _logger->debug("Opening pipe for notifying master.", _worker_id, _socket_fd);
    int result = pipe(_notification_fd);
    if (result < 0) {
        _logger->error("Error opening notification pipe for master: {0}. Code {1}. Aborting.",
                       strerror(errno), errno);
        return -1;
    }

    _logger->debug("Register file descriptor {0} for monitoring incoming request.", _socket_fd, _socket_fd);
    //clear the socket set
    FD_ZERO(&_readfds);
    //if valid socket descriptor then add to read list
    if (_socket_fd > 0) {
        FD_SET(_socket_fd, &_readfds);
    }
    else {
        _logger->error("File descriptor for registering was set to {0}!", _socket_fd);
        return -2;
    }


    // Starting the listener
    std::thread t(&ServerWorker::serve, this, std::ref(_done));

    // Detach the thread
    t.detach();
    _logger->info("Worker {0} started. Ready for incoming connections!", _worker_id);
    _done = false;
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

    int buf_len = 4096, valread = 0;

    _logger->info("Worker {0} is listening now.", _worker_id);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while(true) {

        _lock_thread.lock();

        char *buffer = new char [buf_len];
        done = false;

        // Wait for incoming requests
        activity_on_descriptor = select(_socket_fd + 1, &_readfds,  NULL , NULL , NULL);

        if ((activity_on_descriptor < 0) && (errno!=EINTR)) {
            _logger->error("Select error: {0}. Code {1}.", strerror(errno), errno);
        }

        if (FD_ISSET(_socket_fd, &_readfds)) {
            if ((valread = read(_socket_fd, buffer, buf_len)) == 0) {
                // Somebody disconnected , get his details and print
                done = true;
                handleDisconnectClientRequest();
                delete[] buffer; // Anyway, delete the buffer
                return -1;
            }
            else {
                // Create a new Pointer to a ServerWorker, this will lock the thread!
                std::string server_receive = bufToString(buffer);

                if(server_receive.empty())
                    _logger->debug("Message content was zero");

                auto svr_msg = std::make_shared<ServerMessage>(this, server_receive);
                _logger->info("Created message <{0}-{1}> with content <{2}>",
                               svr_msg->timestamp_str(), svr_msg->messageID_str(),
                               svr_msg->content() );

                _master_svr->message_push(svr_msg);

                delete[] buffer; // Anyway, delete the buffer

            }
        }


    }
#pragma clang diagnostic pop
   // p.set_value(0);
}

int ServerWorker::handleDisconnectClientRequest() {

    std::unique_lock<std::mutex> lock(_lock_disconnect);
    _logger->info("Received disconnect command on worker {0}/descriptor {1}. Shutting down worker.",
                   _worker_id, _socket_fd);

    int addrlen = sizeof(_address);

    // returns the address of the peer connected to the socket sockfd, in the buffer pointed to by addr.
   if (getpeername(_socket_fd, (struct sockaddr *) &_address, (socklen_t*) &addrlen) < 0){
       _logger->error("Can't retrieve peer name: {0}. Code {1}", strerror(errno), errno);
       _logger->info("Host disconnected.", _socket_fd);
   }
   else{
       // inform user of socket number - used in send and receive commands
       _logger->info("Host {0}:{1} disconnected. Remove descriptor {2}." ,
                     inet_ntoa(_address.sin_addr),
                     ntohs(_address.sin_port),
                     _socket_fd);
   }


    // Close the socket and mark as 0 in list for reuse
    if (close(_socket_fd) < 0 ){
        _logger->error("Can't close socket {0}: {1}. Code {2}", _socket_fd, strerror(errno), errno);
    }


    // Delete the logger, thread is about to end
    spdlog::drop(_logger_name);


    // Set the atomic value to true to get the thread's state
    notifiyMaster(NTFY_WKR_DISCON_RCV);
    _lock_disconnect.unlock();

    return 0;

}

int ServerWorker::notifiyMaster(std::string message){

    _lock_notify.lock();
    _logger->debug("Notifying Master about the disconnect.");
    write(_notification_fd[1], message.c_str(), (strlen(message.c_str()) + 1));
    _lock_notify.unlock();
    return 0;
}

std::string ServerWorker::bufToString(char *buffer) {

    std::string ret = "";
    int i = 0;
    if (buffer == nullptr)
        throw 1;


    // Search for a nullterminating char
    while(buffer[i] != '\n') {
        ret += buffer[i++];
    }
    return ret;

}




ServerWorker::~ServerWorker() {


    _logger->debug("Worker {0} terminated. I'll be back.", _worker_id);
    close(_notification_fd[1]);
    close(_notification_fd[0]);

}
