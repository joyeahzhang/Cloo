// copied from muduo/net/tests/TimerQueue_unittest.cc

#include "../net/include/EventLoop.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <iomanip>
#include <functional>

int cnt = 0;
std::shared_ptr<Cloo::EventLoop> g_loop;

void printTid()
{
  std::cout <<"pid: " << getpid() <<", tid: "<<std::this_thread::get_id()<<std::endl;
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::cout<<"now: "<< std::ctime(&now) <<std::endl;
}

void print(const char* msg)
{
  auto time_point = std::chrono::system_clock::now();
  std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()) % 1000;
  std::cout << "Msg: " << std::put_time(std::localtime(&time_t), "%F %T") << "." << std::setfill('0') << std::setw(3) << ms.count() 
            << " " << msg << std::endl;
  if (++cnt == 20)
  {
    g_loop->Quit();
  }
}

int main()
{
  printTid();
  auto loop = Cloo::EventLoop::Create();
  g_loop = loop;

  print("main");
  loop->RunAfter(1000, std::bind(&print, "once1"));
  loop->RunAfter(1500, std::bind(&print, "once1.5"));
  loop->RunAfter(2500, std::bind(&print, "once2.5"));
  loop->RunAfter(3500, std::bind(&print, "once3.5"));
  loop->RunEvery(2000, std::bind(&print, "every2"));
  loop->RunEvery(3000, std::bind(&print, "every3"));

  loop->Loop();
  print("main loop exits");
  sleep(1);
}
