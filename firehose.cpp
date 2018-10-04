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
#include <getopt.h>

#include "safefd.h"

using commune::SafeFD;
using std::string;

static const char *argv0;
static bool verbose = false;

struct V4Addr {
    struct sockaddr_in sockaddr_;

    V4Addr() {
        memset(&sockaddr_, 0, sizeof(sockaddr_));
        sockaddr_.sin_family = AF_INET;
    }

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

// Like perror(), but returns the string suitable for constructing a
// std::exception rather than printing it.
static std::string perr(const char *msg) {
    return std::string(msg) + ": " + strerror(errno);
}

struct Socket {
    SafeFD fd_;

    explicit Socket(int fd):
        fd_(fd)
    {}

    explicit Socket(int domain, int type, int protocol = 0) {
        fd_ = SafeFD(socket(AF_INET, type, 0));

        if (fd_ == -1)
            throw std::runtime_error(string("socket: ") + strerror(errno));

        const int yes = 1;
        if (setsockopt(fd_.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
            throw std::runtime_error(perr("setsockopt(SO_REUSEADDR)"));
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

static void mcast_loop(SafeFD& fd, bool loop) {
    int enable = loop;
    int err = setsockopt(fd.get(), IPPROTO_IP, IP_MULTICAST_LOOP, &enable, sizeof(enable));
    if (err == -1)
        throw std::runtime_error(perr("IP_MULTICAST_LOOP change failed"));
}

static void subscribe(SafeFD& fd, const V4Addr& mcast_addr, const V4Addr& source_addr, const V4Addr& local_addr) {
    int err;

    if (local_addr.sockaddr_.sin_addr.s_addr != INADDR_ANY) {
        err = setsockopt(fd.get(), IPPROTO_IP, IP_MULTICAST_IF, &local_addr.sockaddr_.sin_addr, sizeof(local_addr.sockaddr_.sin_addr));

        if (err == -1)
            throw std::runtime_error(perr("Failed to set multicast interface on listener"));
    }

    struct ip_mreq_source mreq;

    memset(&mreq, 0, sizeof(mreq));

    mreq.imr_multiaddr = mcast_addr.sockaddr_.sin_addr;
    mreq.imr_sourceaddr = source_addr.sockaddr_.sin_addr;

    err = setsockopt(fd.get(), IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &mreq, sizeof(mreq));
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
        argv0 << " [options] fwd 'mcaddr:port' 'src:port' 'unidst:port' ['mc_local_if_addr:ignored']\n";
    std::cerr <<
        argv0 << " [options] recv 'recvaddr:port' 'multidest:port' 'srcaddr:port'\n";
    std::cerr << "\n";
    std::cerr << "options:\n";
    std::cerr << "-v --verbose Verbose output (log each packet)\n";
    std::cerr.flush();
    exit(1);
}

static void flow(Socket& src, Socket& sink) {
    std::cerr << "Beginning traffic flow" << std::endl;

    // 1500: max Ethernet frame size
    // 20: IPv4 header size (IPv6 is 40)
    // 8: UDP header size
    static const size_t BUFSZ = 1500 - 20 - 8;
    char buf[BUFSZ];

    for (;;) {
        ssize_t len = recv(src.fd_.get(), buf, sizeof(buf), 0);

        if (len == -1) {
            throw std::runtime_error(perr("Read error"));
        }

        if (len == 0)
            return;

        if (verbose)
            std::cerr << "Read  " << len << " octets" << std::endl;

        ssize_t wlen = write(sink.fd_.get(), buf, len);
        if (wlen != len)
            std::cerr << perr("Write error") << std::endl;

        if (verbose)
            std::cerr << "Wrote " << wlen << " octets" << std::endl;
    }
}

// Socktype is the type of the transport/tunnel.
// Input is always UDP (multicast).
static void forward(int socktype, const std::vector<std::string>& args) {
    if (args.size() < 4)
        usage();

    V4Addr mcaddr(args[1]);
    V4Addr srcaddr(args[2]);
    V4Addr dest(args[3]);
    V4Addr localaddr;

    if (args.size() >= 5)
        localaddr = V4Addr(args[4]);

    Socket source(AF_INET, SOCK_DGRAM);

    V4Addr bindaddr;
    bindaddr.sockaddr_.sin_port = mcaddr.sockaddr_.sin_port;

    source.bind(bindaddr);
    subscribe(source.fd_, mcaddr, srcaddr, localaddr);
    // for debug
    mcast_loop(source.fd_, true);

    Socket sink(AF_INET, socktype);

    sink.connect(dest);

    flow(source, sink);
}

// Socktype is the type of the transport/tunnel
// output is always UDP.
static void receive(int socktype, const std::vector<std::string>& args) {
    // bind address must be specified at present
    if (args.size() < 4)
        usage();

    V4Addr recvaddr(args[1]);
    V4Addr multiaddr(args[2]);
    V4Addr sendfromaddr(args[3]);

    Socket source(AF_INET, socktype);
    Socket sink(AF_INET, SOCK_DGRAM);

    source.bind(recvaddr);

    sink.bind(sendfromaddr);
    sink.connect(multiaddr);

    flow(source, sink);
}

int main(int argc, char *argv[]) {
    argv0 = argv[0];

    const char optstring[] = "v";
    const struct option longopts[] = {
        { "verbose", no_argument, nullptr, 'v' },
        { nullptr, 0, nullptr, 0 }
    };

    const int socktype = SOCK_DGRAM;

    int opt;
    while ((opt = getopt_long(argc, argv, optstring, longopts, nullptr)) != -1) {
        switch (opt) {
        case 'v':
            verbose = true;
            break;

        default:
        case '?':
            usage();
        }
    }

    std::vector<std::string> args = parseArgs(argc - optind, argv + optind);

    if (args.size() < 1)
        usage();

    if (args[0] == "fwd" || args[0] == "forward")
        forward(socktype, args);
    else if (args[0] == "recv" || args[0] == "receive")
        receive(socktype, args);
    else
        usage();

    return 0;
}
