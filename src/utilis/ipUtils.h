
#ifndef VGEATWAY_IPUTILS_H
#define VGEATWAY_IPUTILS_H

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <netdb.h>
#include "utilis/ipUtils.h"
#include <arpa/inet.h>


static bool isValidIPv4(const std::string& ipAddress) {
  
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


static bool s_valid_ipv6(const std::string& ip) {
    struct sockaddr_in6 sa{};
    return inet_pton(AF_INET6, ip.c_str(), &(sa.sin6_addr)) != 0;
}


static std::tuple<std::string, std::string, int> getHostAndIP() {
    char host[256];
    char *ip;
    struct hostent *host_entry;
    int hostname;

    hostname = gethostname(host, sizeof(host));
    if (hostname == -1) {
        return std::make_tuple(std::strerror(errno), "", -1);
    }

    host_entry = gethostbyname(host);
    if (host_entry == NULL) {
        return std::make_tuple(std::strerror(errno), "", -1);
    }

    ip = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    if (ip == NULL) {
        perror("inet_ntoa");
        return std::make_tuple(std::strerror(errno), "", -1);
    }

    return std::make_tuple(host, ip, 0);
}

#endif //VGEATWAY_IPUTILS_H
