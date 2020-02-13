//
// Created by developer on 08.02.20.
//

#ifndef MULTITCPSERVER_SERVERWORKER_H
#define MULTITCPSERVER_SERVERWORKER_H

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "Server.h"


#include <exception>
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <netinet/in.h>

#include "Notifications.h"


namespace mServer {


    class ServerWorker {

    public:

        // Constructor
        //ServerWorker(int worker_id, int socket_fd, struct sockaddr_in &address);

        // Constructor
        ServerWorker(Server *master_svr, int worker_id, int socket_fd);

        bool hasEnded();

        int initWorker();


        // Getter
        int socket_fds() { return _socket_fd; };

        int notification_fds() { return _notification_fd[0]; };

        int worker_id() { return _worker_id; };

        void lock() { _lock_thread.lock(); };

        void unlock() { _lock_thread.unlock(); };

        // Destructor
        ~ServerWorker();

    private:

        Server *_master_svr;

        std::string _logger_name = "Generic Logger";

        // Initialize with 0,0
        int _notification_fd[2] = {0, 0};
        // ID of the worker
        int _worker_id = 0;

        // Initialize with 0
        std::atomic<int> _socket_fd{0};

        // For determining if a thread has ended.
        std::atomic<bool> _done{false};

        // For logging any kind of messages
        std::shared_ptr<spdlog::logger> _logger;

        fd_set _readfds;

        struct sockaddr_in _address;

        std::mutex _lock_disconnect;

        std::mutex _lock_notify;

        std::mutex _lock_thread;


        int createLogger();

        int serve(std::atomic<bool> &done);

        int handleDisconnectClientRequest();

        int notifiyMaster(std::string message);

        std::string bufToString(char *buffer);
    };
}

#endif //MULTITCPSERVER_SERVERWORKER_H
