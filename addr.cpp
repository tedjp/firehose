#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring>
#include <stdexcept>
#include <utility>

#include "addr.h"

using namespace std;

static pair<string, string> splitAddressString(const string& s) {
    // Look for IPv6 (eg. "[::1]:1234")
    size_t pos = s.find(']');

    if (pos != s.npos)
        return {s.substr(1, pos - 2), s.substr(pos + 2)}; // maybe throws

    // Maybe IPv4
    pos = s.find(':');
    return {s.substr(0, pos), s.substr(pos + 1)};
}

#if 0
V4Addr::V4Addr(const string& s) {
    memset(&sockaddr_, 0, sizeof(sockaddr_));

    auto col = s.find(':');

    if (col == s.npos)
        throw std::runtime_error("port is required");

    string addrpart = s.substr(0, col);
    string portpart = s.substr(col + 1);

    sockaddr_.sin_family = AF_INET;
    unsigned long ulport = std::stoul(portpart);
    if (ulport > std::numeric_limits<uint16_t>::max())
        throw std::range_error("Port out of range");

    sockaddr_.sin_port = htons(static_cast<uint16_t>(ulport));

    if (inet_pton(AF_INET, addrpart.c_str(), &sockaddr_.sin_addr) != 1)
        throw std::runtime_error("Cannot parse v4 addr");
}
#endif

V6Addr::V6Addr() {
    memset(&sockaddr_, 0, sizeof(sockaddr));
    sockaddr_.sin6_family = AF_INET6;
}

V6Addr::V6Addr(const string& s) {
    memset(&sockaddr_, 0, sizeof(sockaddr));
    sockaddr_.sin6_family = AF_INET6;

    struct addrinfo hints = {};

    hints.ai_flags =
          AI_V4MAPPED
        | AI_ALL
        | AI_NUMERICHOST
        | AI_NUMERICSERV;
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;

    pair<string, string> addrparts = splitAddressString(s);

    const string& addrpart = addrparts.first;
    const string& portpart = addrparts.second;

    struct addrinfo *addrs = nullptr;

    int gaierr = getaddrinfo(
            addrpart.c_str(),
            portpart.c_str(),
            &hints,
            &addrs);

    if (gaierr)
        throw runtime_error(string("Address resolution failed: ") + gai_strerror(gaierr));

    if (addrs == nullptr)
        throw runtime_error("Address resolution failed.");

    memcpy(&sockaddr_, addrs->ai_addr, addrs->ai_addrlen);

    freeaddrinfo(addrs);
}
