//
// Created by Christoph Schmidt on 12.02.20.
//

#ifndef MULTITCPSERVER_SERVERMESSAGE_H
#define MULTITCPSERVER_SERVERMESSAGE_H

#include "ServerWorker.h"
#include "ServerExceptions.hpp"
#include "string"
#include <chrono>
#include <ctime>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

//class ServerWorker;

namespace mServer {



    class ServerMessage {

    PortNotValid PortNotValidException;
    ServerNotInit ServerNotInitException;
    DescriptorInvalid DescriptorInvalidException;
    MsgRespondDescriptorNotValid MsgRespondDescriptorNotValidException;
    MsgRespondSendErrorOccurred MsgRespondSendErrorOccurredException;

    public:
        ServerMessage(ServerWorker *parentalWorker, const std::string &msg);

        int respond(std::string &msg);

        boost::uuids::uuid messageID(){ return _message_id;};

        std::string messageID_str(){ return boost::lexical_cast<std::string>(_message_id); };

        time_t timestamp(){ return _time_stamp;};

        std::string timestamp_str();

        std::string &content(){return _content; }

        int workerID(){ return _worker_id;};

        void skip();

        ~ServerMessage();

    private:
        ServerWorker *_parentalWorker;

        // Initialize with 0
        int _socket_fd = 0;

        // Initialize with 0
        int _worker_id = 0;

        std::string _content = "";

        boost::uuids::uuid _message_id{0};

        time_t _time_stamp{0};


        int fd_is_valid(int fd);

        std::string generateMessageID();
    };

}

#endif //MULTITCPSERVER_SERVERMESSAGE_H
