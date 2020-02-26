#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[]) {
    struct sockaddr_in6 addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(1234);

    if (inet_pton(AF_INET6, "ff02::1234", &addr.sin6_addr) != 1)
        return 1;

    int s = socket(PF_INET6, SOCK_DGRAM, 0);
    if (s == -1)
        return 2;

#if 0 // Multicast TTL is not set in IPv6. Implicit based on address.
# if 0
    const int yes = 1;
    if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &yes, sizeof(yes)) == -1) {
        perror("setsockopt multicast ttl");
        return 4;
    }
# else
    int ttl = 0;
    socklen_t ttllen = sizeof(ttl);

    if (getsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_TTL, &ttl, &ttllen) == -1) {
        perror("getsockopt");
        return 6;
    }

    printf("multicast ttl: %d\n", ttl);
# endif
#endif

    struct sockaddr_in6 sendfrom;

    memset(&sendfrom, 0, sizeof(sendfrom));

    sendfrom.sin6_family = AF_INET6;
    sendfrom.sin6_port = htons(4444);
    if (argc < 2) {
        fprintf(stderr, "must provide bind address (not port) in argv[1]\n");
        return 4;
    }

    if (inet_pton(AF_INET6, argv[1], &sendfrom.sin6_addr) != 1) {
        fprintf(stderr, "sender address not parsed\n");
        return 7;
    }

    if (bind(s, (const struct sockaddr*)&sendfrom, sizeof(sendfrom)) == -1) {
        perror("bind");
        return 5;
    }

    const char msg[] = "Hello world\n";
    const size_t msglen = sizeof(msg) - 1;

    ssize_t len = sendto(s, msg, msglen, 0, (const struct sockaddr*)&addr, sizeof(addr));

    printf("sendto: %zd\n", len);

    return len == msglen ? 0 : 3;
}
