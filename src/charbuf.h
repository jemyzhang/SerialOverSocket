//
// Created by jemyzhang on 17-6-13.
//

#ifndef SERIAL_OVER_SOCK_CHARBUF_H
#define SERIAL_OVER_SOCK_CHARBUF_H

#include <mutex>
#include <string>
#include <vector>

using namespace std;

namespace SerialOverSocket {
class CharBuf {
public:
  CharBuf();
  ~CharBuf();

public:
  void reset();
  ssize_t append(const char *content, ssize_t length);
  string getline();
  bool getchar(char &c);
  bool empty();
  string popline();
  char popchar();

private:
  vector<char>::iterator find(char);

private:
  vector<char> m_buf;
  mutex mutex_;
};
}

#endif // SERIAL_OVER_SOCK_CHARBUF_H
