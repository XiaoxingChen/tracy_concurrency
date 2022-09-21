#if !defined(_CONCURRENT_SERVER_H_)
#define _CONCURRENT_SERVER_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <condition_variable>
#include <atomic>
#include <thread>


#include <unistd.h>
#include <array>
#include <string>
#include <queue>

#include "socket_utils.h"
#include "thread_safe_cout.h"
#include "begin_end_proto.h"

namespace trc
{

class ConcurrentServer
{
public:
    ConcurrentServer(int port_num=9090, std::string ip_addr=std::string("127.0.0.1"))
        :enable_flag_(true), port_num_(port_num), ip_addr_(ip_addr)
    {

    }

    ~ ConcurrentServer()
    {}

    void run()
    {
        std::mutex create_thread_mtx;
        // create_thread_mtx.lock();
        // std::condition_variable create_thread_cv;
        std::atomic<size_t> client_num(0);

        // lock.lock();

        while (enable_flag_.load())
        {
            // std::unique_lock<std::mutex> lock(create_thread_mtx);
            create_thread_mtx.lock();
            if(client_num < MAX_CLIENT_NUM)
            {
                client_num++;
                std::thread th([&](){
                    ConcurrentServer::perClientThread(
                        listen_sock_fd_,
                        client_num,
                        create_thread_mtx);
                });
                th.detach();
            }

            // spin_thread_cv_.wait(lock, [&](){ return !enable_flag_.load();});
        }
    }

    void shutdown()
    {
        enable_flag_.store(false);
        ::shutdown(listen_sock_fd_, SHUT_RDWR);
        close(listen_sock_fd_);
        spin_thread_cv_.notify_one();
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

    static void perClientThread(
        int listen_sock_fd,
        std::atomic<size_t>& client_num,
        std::mutex& create_thread_mtx )
    {
        ThreadSafeCout() << "client num: " << client_num.load() << std::endl;

        sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        int sock_fd = accept(listen_sock_fd, (struct sockaddr*)&peer_addr, &peer_addr_len);
        create_thread_mtx.unlock();
        // cv.notify_one();
        if(sock_fd < 0)
        {
            perror("Accept error");
            client_num--;
            return;
        }
        ThreadSafeCout() << "client connected: "
            << ipAddrToString(peer_addr.sin_addr.s_addr)  << ":" << peer_addr.sin_port
            << std::endl;

        std::vector<char> local_buffer(1024);
        std::vector<char> process_buffer;
        std::queue<char> que;
        while(1)
        {
            int len = recv(sock_fd, local_buffer.data(), local_buffer.size(), 0);
            if (len == 0)
            {
                ThreadSafeCout() << "client " << peer_addr.sin_port << " disconnected" << std::endl;
                break;
            }
            if (len < 0)
            {
                ThreadSafeCout() << "Receive error: " << strerror(errno) << std::endl;
                break ;
            }
            for(size_t i = 0; i < len; i++) que.push(local_buffer.at(i));

            while(!que.empty())
            {
                char c = que.front();
                que.pop();
                if(c == BeginEndProto::MSG_HEAD)
                {
                    process_buffer.clear();
                }else if(c == BeginEndProto::MSG_END)
                {
                    for(auto & c : process_buffer) c += 1;
                    int ret = send(sock_fd, process_buffer.data(), process_buffer.size(), 0);
                    if(ret < 1)
                    {
                        ThreadSafeCout() << "send error" << std::endl;
                    }
                }else
                {
                    process_buffer.push_back(c);
                }
            }

        }
        close(sock_fd);
        client_num--;

    }

private:
    static const size_t MAX_CLIENT_NUM = 5;
    std::atomic<bool> enable_flag_;
    std::queue<char> que_;
    std::vector<char> process_buffer_;

    std::mutex state_mtx_;
    std::string ip_addr_ = "127.0.0.1";
    int port_num_ = 9090;
    int listen_sock_fd_ = -1;
    std::vector<int> sock_fds_;
    std::condition_variable spin_thread_cv_;

    // size_t client_num_ = 0;


};




} // namespace trc


#endif // _CONCURRENT_SERVER_H_
