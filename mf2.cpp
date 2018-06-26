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
#include <unistd.h>

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

static std::string perr(const char *msg) {
    return std::string(msg) + ": " + strerror(errno);
}

struct UDPSocket {
    SafeFD fd_;

    UDPSocket() {
        fd_ = SafeFD(socket(AF_INET, SOCK_DGRAM, 0));

        if (fd_ == -1)
            throw std::runtime_error(string("socket: ") + strerror(errno));
    }

    void connect(const V4Addr& dest) {
        int err = ::connect(
                fd_.get(),
                reinterpret_cast<const struct sockaddr*>(&dest.sockaddr_),
                sizeof(dest.sockaddr_));

        if (err != 0)
            throw std::runtime_error(perr("Connect failed"));
    }

    void bind(const V4Addr& addr) {
        int err = ::bind(fd_.get(),
                reinterpret_cast<const struct sockaddr*>(&addr.sockaddr_),
                sizeof(addr.sockaddr_));

        if (err != 0)
            throw std::runtime_error(perr("Bind failed"));
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
        "Usage:\n" <<
        argv0 << " fwd 'mcaddr:port' 'src:port' 'unidst:port'" << '\n';
    std::cerr <<
        argv0 << " recv 'recvaddr:port' 'multidest:port' 'srcaddr:port'" << std::endl;
    exit(1);
}

static void flow(UDPSocket& src, UDPSocket& sink) {
    char buf[1500];
    for (;;) {
        ssize_t len = read(src.fd_.get(), buf, sizeof(buf));
        if (len == -1) {
            throw std::runtime_error("Read error");
        }

        ssize_t wlen = write(sink.fd_.get(), buf, static_cast<size_t>(len));
        if (wlen != len) {
            throw std::runtime_error("Write error");
        }
    }
}

static void forward(const std::vector<std::string>& args) {
    if (args.size() < 5)
        usage();

    V4Addr mcaddr(args[2]);
    V4Addr srcaddr(args[3]);
    V4Addr dest(args[4]);

    UDPSocket source;

    subscribe(source.fd_, mcaddr, srcaddr);

    UDPSocket sink;

    sink.connect(dest);

    flow(source, sink);
}

static void receive(const std::vector<std::string>& args) {
    // bind address must be specified at present
    if (args.size() < 5)
        usage();

    V4Addr recvaddr(args[2]);
    V4Addr multiaddr(args[3]);
    V4Addr sendfromaddr(args[4]);

    UDPSocket source, sink;

    source.bind(recvaddr);

    sink.bind(sendfromaddr);
    sink.connect(multiaddr);

    flow(source, sink);
}

int main(int argc, char *argv[]) {
    argv0 = argv[0];

    // usage:
    // 'fwd' 'mcaddr:port' 'src:port' 'unidst:port'
    // 'receive' 'recvaddr:port' 'mcaddr:port' '(src:port (bind))'

    std::vector<std::string> args = parseArgs(argc, argv);

    if (args.size() < 2)
        usage();

    if (args[1] == "fwd" || args[1] == "forward")
        forward(args);
    else if (args[1] == "recv" || args[1] == "receive")
        receive(args);
    else
        usage();

    return 0;
}
