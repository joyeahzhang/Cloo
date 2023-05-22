# one loop per thread
在Cloo的设计中，遵循one loop per thread设计原则，其含义是每个IO线程拥有一个EventLoop对象。
根据这个设计理念，每个EventLoop对象在构造函数中会检查当前线程是否已经拥有了其他EventLoop对象：
```cpp
// thread local变量记录当前线程已经拥有的EventLoop对象
thread_local EventLoop* T_LOOP_IN_THIS_THREAD = nullptr;

// 在EventLoop构造函数中做检查
EventLoop()
{
    if(T_Loop_THIS_THREAD)
    {
        abort();
    }
    // ...
}
```

# 确保线程安全
Cloo会保证接口的接口的线程安全性，如果一个接口被设计为“不可跨线程调用”
那么函数在实现时，入口处会检查一些条件，以确保接口没有被跨线程调用：
```cpp

// 在某个不可跨线程调用的接口实现中
// 检查是否在创建EventLoop的线程中被调用,如果不是将会结束程序
AssertInLoopThread()

```