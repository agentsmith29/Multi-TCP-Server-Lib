//
// Created by developer on 07.02.20.
//

#ifndef MULTITCPSERVER_SERVEREXCEPTIONS_H
#define MULTITCPSERVER_SERVEREXCEPTIONS_H

#include <exception>
#include <string>

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

    } ErrPortNotValid;

    class SvrNotInit : public exception {

        virtual const char *what() const throw() {
            return "The master server could'nt be initialized. Aborting initialization.";
        }

        virtual const int errID() const throw() {
            return -2;
        }

        virtual const char *errDesc() const throw() {
            return "ERR_SVR_MASTER_NOT_INIT";
        }

    } ErrSvrNotInit;
}

#endif //MULTITCPSERVER_SERVEREXCEPTIONS_H
