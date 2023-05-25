# 问题来源
在Cloo的实现当中, EventLoop和Channel类都继承了 `enable_shared_from_this` 模板类, 从而可以
使用 `shared_from_this()` 方法获取一个指向对象自身的shared_ptr。
但是，如果我们以如下的方式使用 `shared_from_this()`，会导致程序抛出`bad_weak_ptr`异常：
```cpp
class EventLoop : public enable_shared_from_this<EventLoop>
{
    // some data and function
};

int main()
{
    EventLoop loop;
    auto loop_ptr = loop.shared_from_this();
    return 0;
}
```
或者如下这种写法，也会抛出`bad_weak_ptr`异常:

```cpp
class EventLoop : public enable_shared_from_this<EventLoop>
{
public:
    EventLoop()
        : poller_(shared_from_this)
    {

    }
private:
    unique_ptr<Poller> poller_;
};
```
那么这是为什么呢？



# 问题分析
在参考了Stack Overflow上一些相关问答, 和在chatgpt的帮助下, 我了解到对一个对象调用其`shared_from_this()`函数时，要求必须已经存在指向该对象的 `shared_ptr`。`shared_from_this()`只是增加该`shared_ptr`的引用计数，而不会新创建一个`shared_ptr`。
因此回顾刚才两个例子，第一个例子中 loop 是一个栈上对象，没有指向它的`shared_ptr`，我们可以通过以下方式改写：
```cpp
class EventLoop : public enable_shared_from_this<EventLoop>
{
    // some data and function
};

int main()
{
    auto loop = make_shared<EventLoop>();
    auto loop_ptr = loop->shared_from_this();
    return 0;
}
```
第二个例子则更具典型性：对象在构造过程中，绝对不可能存在指向它的`shared_ptr`，因此`shared_from_this()`绝对不能在构造函数中使用。



# 问题解决
虽然我们已经分析出了`bad_weak_ptr`异常的原因，也知道了基本的处理方法(只在存在`shared_ptr`指向对象时，才调用`shared_from_this()`)，但是很多时候我们并不能保证随时都能遵循这些准则(各种原因...)
有没有一种方法能够让我们在使用继承了`enable_shared_from_this`基类的class的object时一定保证已经存在一个`shared_ptr`指向它呢？

根据《Effective Modern C++》Item 19中的建议，我们需要使用做两件事：
    1. 将继承了`enable_shared_from_this`基类的class的构造函数声明为private
    2. 提供一个工厂方法来创建对象，工厂方法返回 `shared_ptr`
例如：
```cpp
class EventLoop : public enable_shared_from_this
{
public:
    static shared_ptr<EventLoop> Create()
    {
        auto loop = make_shared<EventLoop>();
        poller_ = make_unique<Poller>(shared_from_this());
        return loop;
    }

private:
    
    EventLoop() = default;

    unique_ptr<Poller> poller_;
};
```
这个做法很巧妙，也十分好理解：
禁止外部调用EventLoop的构造函数创建栈上对象，避免了**问题来源**一节中的第一个错误。
    
通过工厂函数分两段来create对象，第一段是通过 `make_shared()`函数在堆上创建object和获得一个指向object的`shared_ptr`; 第二段将`shared_ptr`返回给用户，当然这里还将`shared_ptr`提供给了poller_，实现了在创建object过程中使用`shared_from_this`; 避免了**问题来源**一节中的第二个错误。