#if !defined(_TEST_CONCURRENT_SERVER_H_)
#define _TEST_CONCURRENT_SERVER_H_

#include <thread>

#include "concurrent_server.h"
#include "sequential_server.h"
#include "naive_client.h"
#include "logging.h"

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
        if(result.empty())
        {
            trc::ThreadSafeCout() << "buffer empty!" << std::endl;
        }
        for(auto & c : result)
            trc::ThreadSafeCout() << c;
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }


}

inline void concurrentServerTest02()
{
    trc::ThreadSafeCout() << "concurrentServerTest02()" << std::endl;

    int server_port_number = 9092;
    trc::ConcurrentServer server(server_port_number);
    server.init();
    std::thread server_thread([&server](){
        server.run();
    });

#if 1

    std::vector<std::string> messages{"11", "22", "33", "44"};
    std::vector<std::thread> client_threads;
    std::condition_variable start_cv;
    std::mutex start_mtx;
    bool start_flag = false;
    for(size_t i = 0; i < 5; i++)
    {

        // std::promise<std::vector<char>> prom_buff;
        // std::future<std::vector<char>> futu_buff = prom_buff.get_future();

        std::thread client_thread([ i, server_port_number, messages, &start_flag, &start_mtx, &start_cv](){

            std::unique_lock<std::mutex> lock(start_mtx);
            // trc::ThreadSafeCout() << "wait start" << std::endl;
            start_cv.wait(lock, [&start_flag](){return start_flag;});
            lock.unlock();
            // trc::ThreadSafeCout() << "run: " << i << std::endl;

            trc::NaiveClient client(server_port_number);
            client.send(messages);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            client.handleReceiver(true);
            client.shutdown();

            // prom_buff.set_value_at_thread_exit(client.resultBuffer());
            // trc::ThreadSafeCout() << "client thread exit" << std::endl;
            LOGI("client thread exit");
        });

        client_threads.push_back(std::move(client_thread));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    {
        std::unique_lock<std::mutex> lock(start_mtx);
        start_flag = true;
    }
    start_cv.notify_all();

    trc::ThreadSafeCout() << "notified" << std::endl;
    for(auto & th: client_threads)
    {
        th.join();
    }

#endif
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    server.shutdown();
    trc::ThreadSafeCout() << "shutdown server" << std::endl;
    server_thread.join();
}

inline void concurrentServerFullTests()
{
    concurrentServerTest01();
    for(size_t i = 0; i < 1; i++)
    {
        concurrentServerTest02();
    }

    // sequentialServerManualTest01();
    // naiveClientManualTest01();
}

#endif // _TEST_CONCURRENT_SERVER_H_
