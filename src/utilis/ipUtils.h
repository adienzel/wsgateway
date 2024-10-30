
#ifndef VGEATWAY_IPUTILS_H
#define VGEATWAY_IPUTILS_H
#include <iostream>
#include <sstream>
#include <string>
#include <arpa/inet.h>


bool isValidIPv4(const std::string& ipAddress) {
  
    std::vector<std::string> octets;
    std::string octet;
    std::stringstream ss(ipAddress);
    
    // Split the IP address by the '.' delimiter
    while (std::getline(ss, octet, '.')) {
        octets.push_back(octet);
    }
    
    // Check if we have exactly 4 octets
    if (octets.size() != 4) {
        return false;
    }
    
    // Check each octet
    for (const std::string& oct : octets) {
        // Check if the octet is a number and is within the valid range
        if (oct.empty() || oct.size() > 3 || !std::all_of(oct.begin(), oct.end(), ::isdigit)) {
            return false;
        }
        
        // Convert the octet to an integer
        int num = std::stoi(oct);
        if (num < 0 || num > 255) {
            return false;
        }
        
        // Leading zeros are not allowed (e.g., "01" is not valid)
        if (oct[0] == '0' && oct.size() > 1) {
            return false;
        }
    }
    
    return true;
}


bool is_valid_ipv6(const std::string& ip) {
    struct sockaddr_in6 sa{};
    return inet_pton(AF_INET6, ip.c_str(), &(sa.sin6_addr)) != 0;
}


#endif //VGEATWAY_IPUTILS_H
