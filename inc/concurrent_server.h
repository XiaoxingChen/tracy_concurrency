#if !defined(_CONCURRENT_SERVER_H_)
#define _CONCURRENT_SERVER_H_

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

namespace trc
{

class SequentialServer
{
public:
    SequentialServer()
    {}

    ~ SequentialServer()
    {}

    enum ServerState{
        eExit = 0,
        eWaitForClient = 1,
        eWaitForMessage = 2,
        eInMessage = 3
    };

    void run()
    {
        while(1)
        {
            if(state_ == eWaitForClient) runWaitForClient();
            else if(state_ == eWaitForMessage) runWaitForMessage();
            else if(state_ == eInMessage) runInMessage();
            else break;
        }
    }

    void exit()
    {
        if(state_ != eExit)
        {
            close(sock_fd_);
        }
    }

private:
    void runWaitForClient()
    {
        if(listen_sock_fd_ == -1)
        {
            listen_sock_fd_ = listen_inet_socket(ip_addr_, port_num_);
            if(listen_sock_fd_ < 0)
            {
                state_ = eExit;
                return;
            }
        }

        sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        sock_fd_ = accept(listen_sock_fd_, (struct sockaddr*)&peer_addr, &peer_addr_len);
        std::cout << "client connected: " << std::hex
            << peer_addr.sin_addr.s_addr << std::dec << ", " << peer_addr.sin_port << std::endl;
        state_ = eWaitForMessage;
    }

    void runWaitForMessage()
    {
        std::vector<char> local_buffer(1024);
        // int errno;
        int len = recv(sock_fd_, local_buffer.data(), local_buffer.size(), 0);
        if (len == 0)
        {
            std::cout << "client shut down" << std::endl;
            close(sock_fd_);
            state_ = eWaitForClient;
            return;
        }
        if (len < 0) {
            std::cout << "Receive error: " << strerror(errno) << std::endl;
            close(sock_fd_);
            state_ = eExit;
            return ;
        }
        // std::cout << "get message" << std::endl;
        for(size_t i = 0; i < len; i++) que_.push(local_buffer.at(i));

        while(!que_.empty())
        {
            char c = que_.front();
            que_.pop();
            if(c == MSG_HEAD)
            {
                process_buffer_.clear();
            }else if(c == MSG_END)
            {
                state_ = eInMessage;
                break;
            }else
            {
                process_buffer_.push_back(c);
            }
        }


    }

    void runInMessage()
    {
        for(auto & c : process_buffer_) c += 1;
        int ret = send(sock_fd_, process_buffer_.data(), process_buffer_.size(), 0);
        if(ret < 1)
        {
            std::cout << "send error" << std::endl;
        }
        state_ = eWaitForMessage;
    }

private:
    std::queue<char> que_;
    std::vector<char> process_buffer_;
    ServerState state_ = eWaitForClient;
    std::string ip_addr_ = "127.0.0.1";
    int port_num_ = 9091;
    int listen_sock_fd_ = -1;
    int sock_fd_ = -1;
    static const char MSG_HEAD = '^';
    static const char MSG_END = '$';

};



} // namespace trc


#endif // _CONCURRENT_SERVER_H_
