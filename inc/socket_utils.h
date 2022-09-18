#if !defined(_SOCKET_UTILS_H_)
#define _SOCKET_UTILS_H_

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace trc
{

inline int listen_inet_socket(const std::string& ip_addr, int port_num)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_num); /*converts short to
                                        short with network byte order*/
    addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("Socket creation error");
        return -1;
    }

    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("Bind error");
        close(sock);
        return -1;
    }

    if (listen(sock, 1/*length of connections queue*/) == -1) {
        perror("Listen error");
        close(sock);
        return -1;
    }

    return sock;
}


} // namespace trc

#endif // _SOCKET_UTILS_H_
