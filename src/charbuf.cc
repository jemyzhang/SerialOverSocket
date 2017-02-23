//
// Created by jemyzhang on 17-6-13.
//

#include "charbuf.h"
#include <algorithm>

namespace SerialOverSocket {

CharBuf::CharBuf() { m_buf.clear(); }

CharBuf::~CharBuf() { m_buf.clear(); }

void CharBuf::reset() { m_buf.clear(); }

ssize_t CharBuf::append(const char *content, ssize_t length) {
  if (nullptr == content || length == 0)
    return 0;
  vector<char> c(content, content + length);
  mutex_.lock();
  m_buf.insert(m_buf.end(), c.begin(), c.end());
  mutex_.unlock();
  return length;
}

string CharBuf::getline() {
  auto loc = find('\n');
  string ret;
  mutex_.lock();
  if (loc != m_buf.end()) {
    ret = string(m_buf.begin(), loc);
  } else {
    ret = string();
  }
  mutex_.unlock();
  return ret;
}

bool CharBuf::getchar(char &c) {
  bool bret = false;
  c = '\0';
  mutex_.lock();
  if (!m_buf.empty()) {
    c = m_buf[0];
    bret = true;
  }
  mutex_.unlock();
  return bret;
}

bool CharBuf::empty() {
  bool bret = true;
  mutex_.lock();
  if (!m_buf.empty()) {
    bret = false;
  }
  mutex_.unlock();
  return bret;
}

string CharBuf::popline() {
  auto loc = find('\n');
  string ret;
  mutex_.lock();
  if (loc != m_buf.end()) {
    ret = string(m_buf.begin(), loc);
    m_buf.erase(m_buf.begin(), loc + 1);
  } else {
    ret = string();
  }
  mutex_.unlock();
  return ret;
}

char CharBuf::popchar() {
  char c = '\0';
  mutex_.lock();
  if (!m_buf.empty()) {
    c = m_buf[0];
    m_buf.erase(m_buf.begin(), m_buf.begin() + 1);
  }
  mutex_.unlock();
  return c;
}

vector<char>::iterator CharBuf::find(char) {
  auto loc = std::find(m_buf.begin(), m_buf.end(), '\n');
  return loc;
}
}
