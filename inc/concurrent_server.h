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
#include "thread_safe_cout.h"

namespace trc
{


namespace BeginEndProto
{
    static const char MSG_HEAD = '^';
    static const char MSG_END = '$';
} // namespace name


class SequentialServer
{
public:
    SequentialServer(int port_num=9090, std::string ip_addr=std::string("127.0.0.1"))
        :enable_flag_(true), port_num_(port_num), ip_addr_(ip_addr)
    {

    }

    ~ SequentialServer()
    {}

    enum ServerState{
        // eExit = 0,
        eWaitForClient = 1,
        eWaitForMessage = 2,
        // eInMessage = 3
    };

    void run()
    {
        while(enable_flag_.load())
        {
            if(state_ == eWaitForClient) runWaitForClient();
            else if(state_ == eWaitForMessage) runWaitForMessage();
        }
        close(sock_fd_);
        close(listen_sock_fd_);
    }

    void shutdown()
    {
        enable_flag_.store(false);
        ::shutdown(sock_fd_, SHUT_RDWR);
        close(sock_fd_);
        ::shutdown(listen_sock_fd_, SHUT_RDWR);
        close(listen_sock_fd_);
    }

    void init()
    {
        listen_sock_fd_ = listen_inet_socket(ip_addr_, port_num_);
        if(listen_sock_fd_ < 0)
        {
            enable_flag_.store(false);
            ThreadSafeCout() << "listen socket create failed " << std::endl;
            return;
        }
        ThreadSafeCout() << "listen socket create success" << std::endl;
    }

private:
    void runWaitForClient()
    {
        if(listen_sock_fd_ == -1)
        {
            init();
        }
        sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        sock_fd_ = accept(listen_sock_fd_, (struct sockaddr*)&peer_addr, &peer_addr_len);
        if(sock_fd_ < 0)
        {
            perror("Accept error");
            return;
        }
        ThreadSafeCout() << "client connected: "
            << ipAddrToString(peer_addr.sin_addr.s_addr)  << ":" << peer_addr.sin_port
            << std::endl;
        state_ = eWaitForMessage;
    }

    void runWaitForMessage()
    {
        std::vector<char> local_buffer(1024);
        // int errno;
        int len = recv(sock_fd_, local_buffer.data(), local_buffer.size(), 0);
        if (len == 0)
        {
            ThreadSafeCout() << "client disconnected" << std::endl;
            close(sock_fd_);
            state_ = eWaitForClient;
            return;
        }
        if (len < 0) {
            ThreadSafeCout() << "Receive error: " << strerror(errno) << std::endl;
            enable_flag_.store(false);
            return ;
        }
        // std::cout << "get message" << std::endl;
        for(size_t i = 0; i < len; i++) que_.push(local_buffer.at(i));

        // state_ = eInMessage;
        runInMessage();

    }

    void runInMessage()
    {
        // ThreadSafeCout() << "runInMessage()" << std::endl;

        while(!que_.empty())
        {
            char c = que_.front();
            que_.pop();
            if(c == BeginEndProto::MSG_HEAD)
            {
                process_buffer_.clear();
            }else if(c == BeginEndProto::MSG_END)
            {
                for(auto & c : process_buffer_) c += 1;
                int ret = send(sock_fd_, process_buffer_.data(), process_buffer_.size(), 0);
                if(ret < 1)
                {
                    ThreadSafeCout() << "send error" << std::endl;
                }
            }else
            {
                process_buffer_.push_back(c);
            }
        }

        state_ = eWaitForMessage;
    }

private:
    std::atomic<bool> enable_flag_;
    std::queue<char> que_;
    std::vector<char> process_buffer_;
    ServerState state_ = eWaitForClient;
    std::mutex state_mtx_;
    std::string ip_addr_ = "127.0.0.1";
    int port_num_ = 9090;
    int listen_sock_fd_ = -1;
    int sock_fd_ = -1;

};

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
        ThreadSafeCout() << "client init with port: " << addr.sin_port << std::endl;
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


#endif // _CONCURRENT_SERVER_H_
