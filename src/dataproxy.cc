//
// Created by jemyzhang on 17-6-14.
//

#include "dataproxy.h"
#include <algorithm>
#include <cassert>

namespace SerialOverSocket {
shared_ptr<DataProxy> DataProxy::_instance = nullptr;

DataProxy::DataProxy() : quit_(false), work_thread_(&DataProxy::worker, this) {
  buffer_serial_.clear();
  buffer_sock_.clear();
  client_sock_.clear();
  client_serial_.clear();
}

DataProxy::~DataProxy() {
  std::unique_lock<std::mutex>(this->mutex_), (this->quit_ = true);
  condition_.notify_one();
  this->work_thread_.join();
}

void DataProxy::worker() {
  for (;;) {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() {
      return !buffer_sock_.empty() || !buffer_serial_.empty() || quit_;
    });
    if (!buffer_sock_.empty()) {
      for (auto &c : client_serial_) {
        c->received(buffer_sock_.data(), buffer_sock_.size());
      }
      buffer_sock_.clear();
    }
    if (!buffer_serial_.empty()) {
      for (auto &c : client_sock_) {
        c->received(buffer_serial_.data(), buffer_serial_.size());
      }
      buffer_serial_.clear();
    }
    if (quit_)
      break;
  }
}

shared_ptr<DataProxy> DataProxy::getInstance() {
  if (!_instance) {
    _instance = make_shared<DataProxy>();
  }
  return _instance;
}

void DataProxy::RegisterClient(DataProxy::Client *c) {
  assert(c != nullptr);
  lock_guard<mutex> guard(mutex_);
  vector<Client *> *pvect = &client_serial_;
  if (c->type_ == Client::SOCK2SERIAL) {
    pvect = &client_sock_;
  }
  pvect->insert(pvect->end(), c);
}

void DataProxy::DeregisterClient(DataProxy::Client *c) {
  assert(c != nullptr);
  lock_guard<mutex> guard(mutex_);
  vector<Client *> *pvect = &client_serial_;
  if (c->type_ == Client::SOCK2SERIAL) {
    pvect = &client_sock_;
  }
  for (auto i = pvect->begin(); i != pvect->end(); i++) {
    if (*i == c) {
      pvect->erase(i);
      break;
    }
  }
}

void DataProxy::append_serial_input(const char *content, ssize_t length) {
  if (nullptr == content || length == 0)
    return;
  vector<char> c(content, content + length);
  std::unique_lock<std::mutex> guard(mutex_);
  buffer_serial_.insert(buffer_serial_.end(), c.begin(), c.end());
  condition_.notify_one();
}

void DataProxy::append_sock_input(const char *content, ssize_t length) {
  if (nullptr == content || length == 0)
    return;
  vector<char> c(content, content + length);
  std::unique_lock<std::mutex> guard(mutex_);
  buffer_sock_.insert(buffer_sock_.end(), c.begin(), c.end());
  condition_.notify_one();
}

DataProxy::Client::Client(DataProxy::Client::Type t, DataProxy *proxy)
    : type_(t),
      proxy_(proxy == nullptr ? DataProxy::getInstance().get() : proxy) {
  assert(proxy_ != nullptr);
  proxy_->RegisterClient(this);
}

DataProxy::Client::~Client() { proxy_->DeregisterClient(this); }

void DataProxy::Client::transfer(const char *content, ssize_t length) {
  if (type_ == SERIAL2SOCK) {
    proxy_->append_serial_input(content, length);
  }
  if (type_ == SOCK2SERIAL) {
    proxy_->append_sock_input(content, length);
  }
}
} // namespace SerialOverSocket
