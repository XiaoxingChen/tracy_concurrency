#if !defined(_NAIVE_CLIENT_H_)
#define _NAIVE_CLIENT_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>


#include <unistd.h>
#include <array>
#include <string>
#include <queue>

#include "socket_utils.h"
#include "thread_safe_cout.h"

namespace trc
{

class NaiveClient
{
public:
    NaiveClient(int server_port,
        std::string server_ip=std::string("127.0.0.1"),
        std::string client_ip=std::string("127.0.0.1"))
        :server_port_(server_port), server_ip_addr_(server_ip), client_ip_addr_(client_ip)
    {

    }

    ~NaiveClient()
    {
        close(sock_fd_);
    }

    void initConnection()
    {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port_); /*converts short to
                                            short with network byte order*/
        // ThreadSafeCout() << "raw port: " << addr.sin_port << std::endl;
        addr.sin_addr.s_addr = inet_addr(server_ip_addr_.c_str());

        sock_fd_ = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_fd_ == -1) {
            perror("Socket creation error");
            return ;
        }

        if (connect(sock_fd_, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        {
            perror("Connection error");
            close(sock_fd_);
            sock_fd_ = -1;
        }
        // ThreadSafeCout() << "client init with port: " << addr.sin_port << std::endl;
    }

    void shutdown()
    {
        ::shutdown(sock_fd_, SHUT_RDWR);
        close(sock_fd_);
    }

    void handleReceiver(bool block=false)
    {
        std::vector<char> local_buffer(1024);
        int flags = 0;
        if(!block) flags |= MSG_DONTWAIT;
        int len = recv(sock_fd_, local_buffer.data(), local_buffer.size(), flags);
        if(len < 0)
        {
            if(block)
            {
                ThreadSafeCout() << "Receive error: " << strerror(errno) << std::endl;
                close(sock_fd_);
            }
            return ;
        }
        for(size_t i = 0; i < len; i++)
            result_buffer_.push_back(local_buffer.at(i));
    }

    void send( const std::vector<std::string>& messages )
    {
        // std::call_once(init_flag_, [this](){this->initConnection();});
        if(sock_fd_ < 0) initConnection();
        // for(size_t i = 0; i < 5 && sock_fd_ < 0; i++, std::this_thread::sleep_for(std::chrono::milliseconds(1)))
        // {
        //     initConnection();
        // }

        for(const auto & msg: messages)
        {
            std::string packed_msg("^");
            packed_msg += (msg + "$");

            int ret = ::send(sock_fd_, packed_msg.c_str(), packed_msg.size(), 0);
            if(ret < 1)
            {
                ThreadSafeCout() << "send error" << std::endl;
            }
        }
        handleReceiver();
    }

    const std::vector<char>& resultBuffer() const
    {
        return result_buffer_;
    }

private:
    std::string server_ip_addr_ = "127.0.0.1";
    std::string client_ip_addr_ = "127.0.0.1";
    std::vector<char> result_buffer_;
    int server_port_;
    int sock_fd_ = -1;
    // std::once_flag init_flag_;
};



} // namespace trc


#endif // _NAIVE_CLIENT_H_
