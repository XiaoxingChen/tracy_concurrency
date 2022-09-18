#if !defined(_TEST_CONCURRENT_SERVER_H_)
#define _TEST_CONCURRENT_SERVER_H_

#include "concurrent_server.h"

inline void concurrentServerTest01()
{
    trc::SequentialServer server;
    server.run();
}

inline void concurrentServerFullTests()
{
    concurrentServerTest01();
}

#endif // _TEST_CONCURRENT_SERVER_H_
