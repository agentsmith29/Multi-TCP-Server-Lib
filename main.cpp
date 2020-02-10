//
// Created by developer on 07.02.20.
//

#include "include/Server.h"



#include <exception>
#include <iostream>
using namespace mServer;
using std::cout;
using std::endl;
using std::exception;

int main() {




    try {
        Server communicationServer = Server(8080);
    }
    catch(exception& e){
        cout << e.what() << endl;
    }

    return 0;
}