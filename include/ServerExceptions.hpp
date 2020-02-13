//
// Created by developer on 07.02.20.
//

#ifndef MULTITCPSERVER_SERVEREXCEPTIONS_HPP
#define MULTITCPSERVER_SERVEREXCEPTIONS_HPP

#include <exception>
#include <string>
#include <sstream>

using std::string;
using std::exception;

namespace mServer {


    class PortNotValid : public exception {

        virtual const char *what() const throw() {
            return "The given port was not valid. Inputs between port 1 to 65534 valid.";
        }

        virtual const int errID() const throw() {
            return -1;
        }

        virtual const char *errDesc() const throw() {
            return "ERR_PORT_NOT_VALID_DESC";
        }

    };

    class ServerNotInit : public exception {

        virtual const char *what() const throw() {
            return "Master server initialization failed. Aborting initialization."; //ss.str().c_str();
        }

        virtual const int errID() const throw() {
            return -2;
        }

        virtual const char *errDesc() const throw() {
            return "ERR_SVR_MASTER_NOT_INIT";
        }

    };

    class DescriptorInvalid: public exception {

    virtual const char *what() const throw() {
        return "The given file descriptor was smaller than zero. Only file descriptors > 0 are allowed";
    }

    virtual const int errID() const throw() {
        return -3;
    }

    virtual const char *errDesc() const throw() {
        return "ERR_DESC_NOT_VALID";
    }

};

    class MsgRespondDescriptorNotValid : public exception {

        virtual const char *what() const throw() {
            return "Could not send message to client. The descriptor seems not to be valid.";
        }

        virtual const int errID() const throw() {
            return -4;
        }

        virtual const char *errDesc() const throw() {
            return "ERR_MSG_NOT_SENT_DESC_NOT_VALID";
        }

    };

    class MsgRespondSendErrorOccurred : public exception {

        virtual const char *what() const throw() {
            std::stringstream ss;
            ss << "Could not send message to client. Error using send(...): " << strerror(errno) << "Code: " <<errno;
            return ss.str().c_str();
        }

        virtual const int errID() const throw() {
            return -5;
        }

        virtual const char *errDesc() const throw() {
            return "ERR_MSG_NOT_SENT_SEND_ERR_OCCURRED";
        }

    };

    class MsgRegistryEmtpy: public exception {

            virtual const char *what() const throw() {
                return "No messages to read, message buffer is empty!";
            }

            virtual const int errID() const throw() {
                return -6;
            }

            virtual const char *errDesc() const throw() {
                return "ERR_MSG_REGISTRY_IST_EMPTY";
            }

    };



}

#endif //MULTITCPSERVER_SERVEREXCEPTIONS_HPP
