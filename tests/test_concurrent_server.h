#if !defined(_TEST_CONCURRENT_SERVER_H_)
#define _TEST_CONCURRENT_SERVER_H_

#include <thread>

#include "concurrent_server.h"
#include "sequential_server.h"
#include "naive_client.h"

inline void naiveClientManualTest01()
{
    trc::ThreadSafeCout() << "naiveClientManualTest01()" << std::endl;
    int server_port_number = 9096;

    std::vector<std::string> messages{"11", "22", "33", "44"};
    std::promise<std::vector<char>> prom_buff;
    std::future<std::vector<char>> futu_buff = prom_buff.get_future();

    std::thread client_thread([&prom_buff, server_port_number, messages](){
        trc::NaiveClient client(server_port_number);
        client.send(messages);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        client.handleReceiver(true);
        client.shutdown();

        prom_buff.set_value_at_thread_exit(client.resultBuffer());
        trc::ThreadSafeCout() << "client thread exit" << std::endl;
    });

    client_thread.join();

}

inline void sequentialServerManualTest01()
{
    trc::ThreadSafeCout() << "sequentialServerManualTest01()" << std::endl;
    int server_port_number = 9094;
    // std::atomic<bool> exit_server(false);
    trc::SequentialServer server(server_port_number);
    std::thread server_thread([&server](){
        server.run();
        trc::ThreadSafeCout() << "server thread exit" << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    trc::ThreadSafeCout() << "shutdown server" << std::endl;
    server.shutdown();
    server_thread.join();
}

inline void concurrentServerTest01()
{
    trc::ThreadSafeCout() << "concurrentServerTest01()" << std::endl;
    int server_port_number = 9095;

    trc::SequentialServer server(server_port_number);
    server.init();
    std::thread server_thread([&server](){
        server.run();
        trc::ThreadSafeCout() << "server exit" << std::endl;
    });

    std::vector<std::string> messages{"11", "22", "33", "44"};
    std::promise<std::vector<char>> prom_buff;
    std::future<std::vector<char>> futu_buff = prom_buff.get_future();

    std::thread client_thread([&prom_buff, server_port_number, messages](){
        trc::NaiveClient client(server_port_number);
        client.send(messages);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        client.handleReceiver(true);
        client.shutdown();

        prom_buff.set_value_at_thread_exit(client.resultBuffer());
        trc::ThreadSafeCout() << "client thread exit" << std::endl;
    });

    client_thread.join();
    server.shutdown();
    server_thread.join();


    // std::cout << "done" << std::endl;
    std::vector<char> result = futu_buff.get();
    std::vector<char> expected_result{'2','2','3','3','4','4','5','5'};

    if(result != expected_result)
    {
        for(auto & c : result)
            trc::ThreadSafeCout() << c;
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }


}

inline void concurrentServerTest02()
{
    trc::ThreadSafeCout() << "concurrentServerTest02()" << std::endl;
    trc::ConcurrentServer server;
    server.init();
    std::thread server_thread([&server](){
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    server.shutdown();
    server_thread.join();
}

inline void concurrentServerFullTests()
{
    concurrentServerTest01();
    concurrentServerTest02();
    // sequentialServerManualTest01();
    // naiveClientManualTest01();
}

#endif // _TEST_CONCURRENT_SERVER_H_
