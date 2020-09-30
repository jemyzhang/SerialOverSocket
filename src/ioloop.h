//
// Created by jemyzhang on 17-6-13.
//

#ifndef SERIAL_OVER_SOCKET_IOLOOP_H
#define SERIAL_OVER_SOCKET_IOLOOP_H

#include "handler.h"
#include <map>
#include <memory>

using namespace std;

namespace SerialOverSocket {
  class IOLoop {
  public:
    IOLoop();

    ~IOLoop();

  public:
    void start();

    void stop() { running_ = false; }

    void addHandler(int fd, Handler *handler, unsigned int events);

    void modifyHandler(int fd, unsigned int events);

    void removeHandler(int fd);

    int get_eventpoll_fd(void) { return epfd_; }

  public:
    static shared_ptr<IOLoop> getInstance();

  private:
    int epfd_;
    bool running_;
    static shared_ptr<IOLoop> instance_;
    map<int, Handler *> handlers_;
  };
}

#endif // SERIAL_OVER_SOCKET_IOLOOP_H
