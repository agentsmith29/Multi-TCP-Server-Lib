//
// Created by developer on 08.02.20.
//

#ifndef MULTITCPSERVER_SERVERWORKER_H
#define MULTITCPSERVER_SERVERWORKER_H

#include "spdlog/spdlog.h"
#include "spdlog/async.h"

#include <exception>
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <netinet/in.h>

namespace mServer{


    class ServerWorker {

        public:
            ServerWorker(int worker_id, int socket_descriptor, struct sockaddr_in &address);

            bool hasEnded();

            int getSocketDescriptor() {
                return _socket_descriptor;
            };

            ~ServerWorker();

        private:
            std::string _logger_name = "Generic Logger";

            int _socket_descriptor = 0;

            std::atomic<bool> _done;

            int _worker_id = 0;

            fd_set _readfds;

            int createLogger();

            //
            int serve(std::atomic<bool> &done);

            int handleDisconnectClientRequest();

            // For logging any kind of messages
            std::shared_ptr<spdlog::logger> _logger;

            //

            struct sockaddr_in _address;

    };
}

#endif //MULTITCPSERVER_SERVERWORKER_H
