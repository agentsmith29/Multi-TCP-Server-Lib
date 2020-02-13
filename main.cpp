//
// Created by developer on 07.02.20.
//

#include "include/Server.h"
#include "include/ServerMessage.h"



#include <exception>
#include <iostream>

using namespace mServer;
using std::cout;
using std::endl;
using std::exception;

int main() {
    Server *communicationServer;

    try {
        communicationServer = new Server(8080);
    }
    catch (exception &e) {
        cout << e.what() << endl;
    }


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while(true) {
        //sleep(5);
        communicationServer->waitForMessage();
        try {
            auto svr_pop = communicationServer->message_front();
            cout << "Replaying to message <" << svr_pop->content() << ">" << std::endl;
            // Directly echo back
            std::string respond = "Received: " + svr_pop->content() + "\n";
            svr_pop->respond(respond); // This will release the lock!
            communicationServer->message_pop();
        }
        catch (exception &e) {
            cout << e.what() << endl;
        }

    }
#pragma clang diagnostic pop

    return 0;
}