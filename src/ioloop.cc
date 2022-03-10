//
// Created by jemyzhang on 17-6-13.
//

#include "ioloop.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace SerialOverSocket {

shared_ptr<IOLoop> IOLoop::instance_ = nullptr;

shared_ptr<IOLoop> IOLoop::getInstance() {
  if (!instance_) {
    instance_ = make_shared<IOLoop>();
  }
  return instance_;
}

IOLoop::IOLoop() {
  epfd_ = epoll_create1(0);
  if (epfd_ < 0) {
    perror("epoll_create");
    exit(1);
  }
  this->handlers_.clear();
}

IOLoop::~IOLoop() = default;

void IOLoop::start() {
#define MAX_EVENTS 1024
  running_ = true;
  epoll_event events[MAX_EVENTS];
  memset(events, 0, sizeof(epoll_event) * MAX_EVENTS);
  for (;;) {
    int nfds = epoll_wait(epfd_, events, MAX_EVENTS, -1);
    for (int i = 0; i < nfds; ++i) {
      int fd = events[i].data.fd;
      Handler *h = handlers_[fd];
      h->handle(events[i]);
    }
    if (!running_) {
      break;
    }
  }
}

void IOLoop::addHandler(int fd, Handler *handler, unsigned int events) {
  handlers_[fd] = handler;
  epoll_event e{};
  e.data.fd = fd;
  e.events = events;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &e) < 0) {
    perror("epoll_ctl ADD");
  }
}

void IOLoop::modifyHandler(int fd, unsigned int events) {
  epoll_event e{};
  e.data.fd = fd;
  e.events = events;
  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &e) < 0) {
    perror("epoll_ctl MOD");
  }
}

void IOLoop::removeHandler(int fd) {
  epoll_event e{};
  e.data.fd = fd;
  if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &e) < 0) {
    perror("epoll_ctl DEL");
  }
}
} // namespace SerialOverSocket
