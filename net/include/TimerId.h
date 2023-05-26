#pragma once

#include "Timer.h"

#include <memory>

namespace Cloo 
{

// class Timer;

class TimerId
{

public:
    TimerId( const std::shared_ptr<Timer>& timer );

    // 含有unique_ptr成员, 不可拷贝

private:
    std::shared_ptr<Timer> timer_;
};

} // end namespace Cloo