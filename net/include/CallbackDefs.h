#pragma once

#include <functional>
namespace Cloo::define 
{

using IOEventCallback = std::function<void()>;
using TimerCallback = std::function<void()>;

} // end namespace Cloo::define