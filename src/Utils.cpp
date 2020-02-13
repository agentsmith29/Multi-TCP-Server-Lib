//
// Created by Christoph Schmidt on 13.02.20.
//


#include "string"

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

std::string& bufToString(char* buffer, int len){

    std::string ret = "test";
    if (buffer == nullptr)
        throw 1;

    int i = 0;
    // Search for a nullterminating char
    /*
    do{
        ret = buffer[i++];
    } while(buffer[i] != '\n');
    */


    return ret;

}
