#if !defined(_THREAD_SAFE_COUT_H_)
#define _THREAD_SAFE_COUT_H_

#include <mutex>
#include <sstream>
#include <iostream>

namespace trc
{

class ThreadSafeCout: public std::ostringstream
{
public:
    ThreadSafeCout(){}

    static std::mutex& getMutex()
    {
        static std::mutex mtx;
        return mtx;
    }

    ~ThreadSafeCout()
    {
        ThreadSafeCout::getMutex().lock();
        std::cout << this->str();
        ThreadSafeCout::getMutex().unlock();
    }


private:

};

} // namespace trc




#endif // _THREAD_SAFE_COUT_H_
