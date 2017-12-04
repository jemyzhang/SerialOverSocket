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
  bool hasline();
  bool getchar(char &c);
  bool empty();
  string popline();
  char popchar();
  //remove one char from the last line expect the LF
  ssize_t remove_line_char(ssize_t length=1);

private:
  vector<char>::iterator find(char);

private:
  vector<char> m_buf;
  mutex mutex_;
};
}

#endif // SERIAL_OVER_SOCK_CHARBUF_H
