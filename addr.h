#pragma once

#include <netinet/in.h>
#include <string>

struct V6Addr {
    struct sockaddr_in6 sockaddr_;
    V6Addr();
    V6Addr(const std::string& s);
};
