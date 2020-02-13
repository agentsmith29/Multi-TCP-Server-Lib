//
// Created by developer on 08.02.20.
//

#ifndef MULTITCPSERVER_UTILS_H
#define MULTITCPSERVER_UTILS_H

#include "string"

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& bufToString(char* buffer, int len);

#endif //MULTITCPSERVER_UTILS_H
