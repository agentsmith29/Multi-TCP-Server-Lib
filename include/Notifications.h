//
// Created by Christoph Schmidt on 12.02.20.
//

#ifndef MULTITCPSERVER_NOTIFICATIONS_H
#define MULTITCPSERVER_NOTIFICATIONS_H

#define NTFY_INIT_SVR_ABRT "ERR_INIT_SVR_ABRT"
#define NTFY_WKR_DISCON_RCV "NTFY_WKR_DISCON_RCV"

#include "string.h"


/*
static constexpr int NOTIFICATION_LEVEL_FATAL = 0;
static constexpr int NOTIFICATION_LEVEL_1 = 1;
static constexpr int NOTIFICATION_LEVEL_2 = 2;
static constexpr int NOTIFICATION_LEVEL_3 = 3;

class Noticiation {
public:
    virtual std::string message() = 0;
    virtual int message_level() = 0;
    virtual int err_type() = 0;
};

class INIT_SVR_ABORT: public Noticiation {

public:
    std::string message() { return _message; };
    int message_level() { return _message_level; };
    int err_type() { return _err_type; };

    bool operator== (INIT_SVR_ABORT &msg2) {
        return ( !std::strcmp( message().c_str(), msg2.message().c_str()) && (err_type() == msg2.err_type()) );
    }
private:

    std::string _message = "ERR_INIT_SVR_ABRT";

    int _message_level = NOTIFICATION_LEVEL_FATAL;

    int _err_type = 11011;


};

*/
#endif //MULTITCPSERVER_NOTIFICATIONS_H
