#include "include/Timer.h"
#include "include/TimeDefs.h"
#include <iostream>

using namespace Cloo;
using namespace Cloo::define;
using namespace std;

Timer::Timer(const TimerCallback& cb, SystemTimePoint when, long interval_ms)
    : callback_(cb),
      expiration_(when),
      interval_ms_(interval_ms),
      repeat_(interval_ms_ > 0)
{

}

Timer::~Timer()
{

}

void Timer::Restart(SystemTimePoint now)
{
    if (repeat_) 
    {
        expiration_ = now + chrono::milliseconds(interval_ms_);
    }
    else
    {
        expiration_ = chrono::time_point<chrono::system_clock>::min();
    }
}
