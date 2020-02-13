//
// Created by Christoph Schmidt on 12.02.20.
//

#include "../include/ServerMessage.h"
#include "../include/ServerExceptions.hpp"
//



#include <unistd.h>
#include <fcntl.h>



using  namespace mServer;

ServerMessage::ServerMessage(ServerWorker *parentalWorker, const std::string &msg) {

    std::cout << msg << std::endl;
    // Assign a timestampe to the message
    _time_stamp = time(0);
    // Create a unique message ID
    generateMessageID();

    _parentalWorker = parentalWorker;
    if(_parentalWorker != nullptr) {
        _socket_fd = _parentalWorker->socket_fds();
        _worker_id = _parentalWorker->socket_fds();

        _content = msg;
    }

}

int ServerMessage::fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

int ServerMessage::respond(std::string &msg) {

    // First check if the descriptor is not zero
    if(_socket_fd == 0){
        _parentalWorker->unlock(); // Anyway, release the lock!
        throw DescriptorInvalidException;
    }
    if(fd_is_valid(_socket_fd) != 1){

        _parentalWorker->unlock(); // Anyway, release the lock!
        throw MsgRespondDescriptorNotValidException;
    }

    //Before sending check if the parental worker is even vailable
   if(_parentalWorker->hasEnded()){
       std::cout << "Parental worker " << _worker_id << " is not available any more."  << std::endl;
       return 0;
   }

    //send new connection greeting message
    if(send(_socket_fd, msg.c_str(), msg.length(), 0) != msg.length() ) {
        throw MsgRespondSendErrorOccurredException;
    }
    _parentalWorker->unlock(); // Anyway, release the lock!
    return 0;
    //************************************
    // DO NOT FORGET TO RELEASE THE LOCK!
    //************************************
}

std::string ServerMessage::generateMessageID(){
    std::string id;
    boost::uuids::random_generator gen;
    _message_id = gen();
    std::cout << "ID:" << _message_id << std::endl;
    return boost::lexical_cast<std::string>(_message_id);
    //std::tm * ptm = std::localtime(&_time_stamp);
    //char buffer[32];
    // Format: Mo, 15.06.2009 20:20:00
    //std::strftime(buffer, 32, "%d%m%Y-%H%M%S", ptm);
    // Worst Case: 31129999235959
    // int         ****2147483647

    //id = _time_stamp;
    //return id;
}

std::string ServerMessage::timestamp_str(){
    std::string timestamp;
    std::tm * ptm = std::localtime(&_time_stamp);
    char buffer[32];
    // Format: Mo, 15.06.2009 20:20:00
    std::strftime(buffer, 32, "%d%m%Y-%H%M%S", ptm);

    return timestamp;

};


ServerMessage::~ServerMessage() {
    std::cout << "Destroyed message."<< std::endl;
}