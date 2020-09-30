//
// Created by jemyzhang on 17-6-14.
//

#ifndef SERIAL_OVER_SOCKET_DATAPROXY_H
#define SERIAL_OVER_SOCKET_DATAPROXY_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

namespace SerialOverSocket {

  class DataProxy {
  public:;

    DataProxy();

    ~DataProxy();

    class Client {
    public:
      enum Type {
        SERIAL2SOCK,
        SOCK2SERIAL,
      };

      explicit Client(Type t, DataProxy *proxy = nullptr);

      ~Client();

      void transfer(const char *, ssize_t);

      virtual void received(const char *, ssize_t) = 0;

    private:
      Type type_;
      DataProxy *proxy_;

      friend class DataProxy;
    };

  public:
    static shared_ptr<DataProxy> getInstance();

    void RegisterClient(Client *);

    void DeregisterClient(Client *);

    void append_serial_input(const char *, ssize_t);

    void append_sock_input(const char *, ssize_t);

  private:
    void worker();

  private:
    std::mutex mutex_;
    condition_variable condition_;
    vector<char> buffer_serial_;
    vector<char> buffer_sock_;
    bool quit_;
    thread work_thread_;
    vector<Client *> client_sock_;
    vector<Client *> client_serial_;

    static shared_ptr<DataProxy> _instance;
  };
}

#endif // SERIAL_OVER_SOCKET_DATAPROXY_H
