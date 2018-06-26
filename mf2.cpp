#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "safefd.h"

using commune::SafeFD;
using std::string;

static const char *argv0;

struct V4Addr {
    struct sockaddr_in sockaddr_;

    V4Addr(const string& s) {
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
};

struct UDPSocket {
    SafeFD fd_;

    UDPSocket() {
        fd_ = SafeFD(socket(AF_INET, SOCK_DGRAM, 0));

        if (fd_ == -1)
            throw std::runtime_error(string("socket: ") + strerror(errno));
    }
};

static void subscribe(SafeFD& fd, const V4Addr& mcast_addr, const V4Addr& source_addr) {
    struct ip_mreq_source mreq;

    memset(&mreq, 0, sizeof(mreq));

    mreq.imr_multiaddr = mcast_addr.sockaddr_.sin_addr;
    mreq.imr_sourceaddr = source_addr.sockaddr_.sin_addr;

    int err = setsockopt(fd.get(), IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err)
        throw std::runtime_error(string("failed to join: ") + strerror(errno));
}

static std::vector<std::string> parseArgs(int argc, char *argv[]) {
    std::vector<std::string> args;

    args.reserve(argc);

    for (int n = 0; n < argc; ++n)
        args.emplace_back(argv[n]);

    return args;
}

__attribute__((noreturn))
static void usage();
static void usage() {
    std::cerr <<
        "Usage: " << argv0 << " forward 'mcaddr:port' 'src:port' 'unidst:port'" << std::endl;
    exit(1);
}

static void forward(const std::vector<std::string>& args) {
    if (args.size() < 5)
        usage();

    V4Addr mcaddr(args[2]);
    V4Addr srcaddr(args[3]);
    V4Addr dest(args[4]);

    UDPSocket s;

    subscribe(s.fd_, mcaddr, srcaddr);
}

static void receive(const std::vector<std::string>& args) {
    return;
}

int main(int argc, char *argv[]) {
    argv0 = argv[0];

    // usage:
    // 'forward' 'mcaddr:port' 'src:port' 'unidst:port'
    // 'receive' 'mcaddr:port' (src:port (bind))'

    std::vector<std::string> args = parseArgs(argc, argv);

    if (args.size() < 2)
        usage();

    UDPSocket s;

    if (args[1] == "forward")
        forward(args);
    else if (args[1] == "receive")
        receive(args);
    else
        usage();

    return 0;
}
