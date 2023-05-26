#include "include/TimerId.h"
#include "include/Timer.h"
#include <memory>

using namespace Cloo;
using namespace std;

TimerId::TimerId( const shared_ptr<Timer>& timer)
    : timer_( timer )
{

}