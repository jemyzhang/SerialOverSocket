//
// Created by jemyzhang on 20-1-21.
//

#ifndef SERIAL_OVER_SOCKET_SNIPPETS_H
#define SERIAL_OVER_SOCKET_SNIPPETS_H

#include <string>
#include <memory>
using namespace std;

namespace SerialOverSocket {
  class Snippets {
  public:
    Snippets() = default;
    ~Snippets() = default;

  public:
    static shared_ptr<Snippets> getInstance();

  public:
    void set_location(string l) {location = l;}
    string ls(string prefix = string());
    bool exists(string title);
    string cat(string title, string prefix = string());

  public:
    static shared_ptr<Snippets> instance_;
  private:
    string location;
  };
} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_SNIPPETS_H
