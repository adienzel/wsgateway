#ifndef VGATEWAY_SPLIT_H
#define VGATEWAY_SPLIT_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

static std::vector<std::string> split(const std::string& str, char delimiter = ',') {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

#endif //VGATEWAY_SPLIT_H
