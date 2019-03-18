//
// Created by Edward Hyde on 17/03/2019.
//

#ifndef CPP_HTTP_RANGE_FILESERVER_RANGE_H
#define CPP_HTTP_RANGE_FILESERVER_RANGE_H

#include <string>
#include <fstream>
#include "reply.hpp"

class range {
    static const int DEFAULT_BUFFER_SIZE = 204800;
public:
    unsigned long start;
    unsigned long end;
    unsigned long length;
    unsigned long total;
    range(unsigned long start, unsigned long end, unsigned long total) :
    start(start),
    end(end),
    length(end - start + 1),
    total(total) {}

    static long sublong(const std::string& value, unsigned long beginIndex, unsigned long endIndex){
        std::string substring = value.substr(beginIndex, endIndex);
        return (substring.length() > 0) ? std::stol(substring) : -1;
    }

    static void copy(std::ifstream& is, http::server3::reply& rep, unsigned long inputSize, long start, unsigned long length){
        char buffer[DEFAULT_BUFFER_SIZE];
        long read;
        if(inputSize == length){
            while((read = is.read(buffer, sizeof(buffer)).gcount()) > 0){
                rep.content.append(buffer, (unsigned long)read);
            }
        } else {
            is.ignore(start);
            unsigned long toRead = length;
            while((read = is.read(buffer, sizeof(buffer)).gcount()) > 0) {
                if((toRead -= read) > 0){
                    rep.content.append(buffer, (unsigned long)read);
                } else{
                    rep.content.append(buffer, toRead + read);
                    break;
                }
            }
        }
    }
};

#endif //CPP_HTTP_RANGE_FILESERVER_RANGE_H
