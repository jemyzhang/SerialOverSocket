//
// Created by jemyzhang on 16-11-28.
//

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <pwd.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "ansi_color.h"
#include "config.h"
#include "connection_server.h"
#include "ioloop.h"
#include "serialport.h"
#include "snippets.h"
#include "sockserver.h"
#include "version.h"

using namespace std::placeholders;

namespace SerialOverSocket {

ConnectionManager::ConnectionManager() { connections_.clear(); }

ConnectionManager::~ConnectionManager() {
  mutex_.lock();
  // TODO: need to free space here?
  connections_.clear();
  mutex_.unlock();
}

int ConnectionManager::append(Connection *pinfo) {
  mutex_.lock();
  connections_.insert(connections_.end(), pinfo);
  mutex_.unlock();
  return 0;
}

Connection *ConnectionManager::pop() {
  Connection *p = nullptr;
  mutex_.lock();
  p = connections_.front();
  connections_.erase(connections_.begin());
  mutex_.unlock();
  return p;
}

Connection *ConnectionManager::remove(int fd) {
  Connection *pret = nullptr;
  mutex_.lock();
  for (auto i = connections_.begin(); i != connections_.end(); i++) {
    Connection *p = *i;
    if (p->fd_ == fd) {
      connections_.erase(i);
      pret = p;
      break;
    }
  }
  mutex_.unlock();
  return pret;
}

int ConnectionManager::remove(Connection *pinfo) {
  int fd = 0;
  if (pinfo) {
    fd = pinfo->fd_;
    remove(fd);
  }
  return fd;
}

Connection *ConnectionManager::find(int fd) {
  Connection *pret = nullptr;
  mutex_.lock();
  for (auto p : connections_) {
    if (p->fd_ == fd) {
      pret = p;
      break;
    }
  }
  mutex_.unlock();
  return pret;
}

ClientConnection::ClientConnection(int fd, string host, string port)
    : Connection(fd, {CONNECTION_CLIENT, host + ":" + port, ""}),
      DataProxy::Client(DataProxy::Client::SOCK2SERIAL) {}

ClientConnection::~ClientConnection() = default;

ssize_t ClientConnection::write_rxbuf(const char *content, ssize_t length) {
  transfer(content, length);
  return length;
}

void ClientConnection::received(const char *content, ssize_t length) {
  write_txbuf(content, length);
}

SerialPortConnection::SerialPortConnection(int fd, string dev, string detail)
    : Connection(fd, {CONNECTION_SERIAL, dev, detail}),
      DataProxy::Client(DataProxy::Client::SERIAL2SOCK) {}

SerialPortConnection::~SerialPortConnection() = default;

ssize_t SerialPortConnection::write_rxbuf(const char *content, ssize_t length) {
  transfer(content, length);
  return length;
}

void SerialPortConnection::received(const char *content, ssize_t length) {
  write_txbuf(content, length);
}

#define MGR_PROMPT_STR "MGR$ "
#define MGR_PROMPT YELLOW MGR_PROMPT_STR NONE
AdminConnection::AdminConnection(int fd, string host, string port)
    : Connection(fd, {CONNECTION_ADMIN, host + ":" + port, ""}),
      quit_(false), DataProxy::Client(DataProxy::Client::SOCK2SERIAL),
      processor_thread_(&AdminConnection::cmd_processor, this) {
  history_.clear();
  string histfile = string(getpwuid(getuid())->pw_dir) + "/.config/sos/history";
  if (access(histfile.c_str(), F_OK) != -1) {
    // load history
    ifstream in(histfile);
    string str;
    while (getline(in, str))
      history_.push_back(str);
  }
  history_idx_ = 0;
  cursor_pos_ = 0;
  write_txbuf("\n");
  write_txbuf(GREEN "===---      Serial Over Socket     ---===\n");
  write_txbuf("===---   Control Panel " UNDERLINE VERSION GREEN "   ---===\n");
  write_txbuf(": Enter \"" YELLOW "help" GREEN "\" for usage hints\n");
  write_txbuf(MGR_PROMPT);
}

AdminConnection::~AdminConnection() {
  std::unique_lock<std::mutex>(this->mutex_), (this->quit_ = true);
  condition_.notify_one();
  processor_thread_.join();
  if (!history_.empty()) {
    // save history
    string histfile =
        string(getpwuid(getuid())->pw_dir) + "/.config/sos/history";
    ofstream of(histfile);
    ostream_iterator<string> oiter(of, "\n");
    copy(history_.begin(), history_.end(), oiter);
  }
}

void AdminConnection::received(const char *content, ssize_t length) {
  // received from serial port, not process
}

ssize_t AdminConnection::write_rxbuf(const char *content, ssize_t length) {
  std::unique_lock<std::mutex> guard(mutex_);
  bool echo = true;
  bool ignore = false;
  // assume input char by char
  if (length == 3) {
    if (content[0] == 0x1b && content[1] == '[') {
      int len_his = history_.size();
      string str_his = string();
      switch (content[2]) {
      case 'A': // up
      case 'B': // down
        echo = false;
        ignore = true;
        if (history_.empty()) {
          break;
        }

        write_txbuf(CLRLINE MGR_PROMPT);
        cmdline_.clear();
        cursor_pos_ = 0;
        if (content[2] == 'A') {
          history_idx_++;
          if (history_idx_ > len_his) {
            history_idx_ = len_his;
          }
        } else {
          history_idx_--;
        }
        if (history_idx_ > 0) {
          cmdline_ = history_.at(len_his - history_idx_);
          write_txbuf(cmdline_);
          cursor_pos_ = cmdline_.length();
        }
        break;
      case 'C': // right
        ignore = true;
        if (cursor_pos_ < cmdline_.length()) {
          cursor_pos_++;
        } else {
          echo = false;
        }
        break;
      case 'D': // left
        ignore = true;
        if (cursor_pos_ > 0) {
          cursor_pos_--;
        } else {
          echo = false;
        }
        break;
      default:
        break;
      }
    }
  } else if (length == 1) {
    switch (content[0]) {
    case '\n':
      rxbuf_.append(cmdline_.c_str(), cmdline_.length());
      rxbuf_.append(content, length);
      condition_.notify_one();
      cmdline_.clear();
      cursor_pos_ = 0;
      history_idx_ = 0;
      write_txbuf(content, length);
      return length;
    case 0x3:
      echo = false;
      ignore = true;
      cmdline_.clear();
      cursor_pos_ = 0;
      write_txbuf("\n" MGR_PROMPT);
      break;
    case 0x7f:
      ignore = true;
      echo = false;
      if (!cmdline_.empty() && cursor_pos_ > 0) {
        cursor_pos_--;
        cmdline_.erase(cursor_pos_, 1);
        write_txbuf(MOVE_LEFT(1) CLRAFTER);
      }
      break;
    default:
      break;
    }
  }

  if (!ignore) {
    cmdline_.insert(cursor_pos_, content, length);
    cursor_pos_ += length;
  }

  write_txbuf(CLRLINE MGR_PROMPT);
  write_txbuf(cmdline_);
  char s[32];
  sprintf(s, "\r\e[%ldC", cursor_pos_ + strlen(MGR_PROMPT_STR));
  write_txbuf(s, strlen(s));

  return length;
}

void AdminConnection::cmd_processor() {
  for (;;) {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() { return rxbuf_.hasline() || quit_; });
    string cmd_input;
    if (!(cmd_input = rxbuf_.popline()).empty()) {
      std::stringstream ss(cmd_input);
      std::istream_iterator<std::string> begin(ss);
      std::istream_iterator<std::string> end;
      std::vector<std::string> args(begin, end);
      bool save_history = true;
      if (args[0] == "help") {
        print_help();
      } else if (args[0] == "history") {
        for (auto &s : history_) {
          write_txbuf(s);
          write_txbuf("\n");
        }
      } else if (args[0] == "disconnect") {
        SerialPort::getInstance()->disconnect();
      } else if (args[0] == "connect") {
        SerialPort::getInstance()->connect();
      } else if (args[0] == "reconnect") {
        SerialPort::getInstance()->reconnect();
      } else if (args[0] == "set") {
        if (args.size() < 3) {
          write_txbuf(
              BOLD_BLUE
              "usage: set baudrate/databits/parity/stopbit [options]\n" NONE);
        } else {
          bool ret = false;
          bool not_supported = false;
          try {
            if (args[1] == "baudrate") {
              int v = stoi(args[2]);
              ret =
                  SerialPort::getInstance()->set_baudrate(v, true);
              if (ret) {
                ServerConfig::getInstance()->serial_baudrate(v);
              }
            } else if (args[1] == "databits") {
              int v = stoi(args[2]);
              ret =
                  SerialPort::getInstance()->set_databits(v, true);
              if (ret) {
                ServerConfig::getInstance()->serial_databits(v);
              }
            } else if (args[1] == "parity") {
              char v = stoi(args[2]);
              ret = SerialPort::getInstance()->set_parity(v, true);
              if (ret) {
                ServerConfig::getInstance()->serial_parity(v);
              }
            } else if (args[2] == "stopbit") {
              int v = stoi(args[2]);
              ret = SerialPort::getInstance()->set_stopbit(v, true);
              if (ret) {
                ServerConfig::getInstance()->serial_stopbit(v);
              }
            } else {
              write_txbuf(BOLD_RED "action not supported\n" NONE);
              not_supported = true;
            }
            if (!not_supported) {
              write_txbuf(ret ? BOLD_GREEN "OK\n" NONE
                              : BOLD_RED "FAIL\n" NONE);
            }
          } catch (std::invalid_argument) {
            write_txbuf(BOLD_RED "FAIL\n" NONE);
          }
        }
      } else if (args[0] == "save") {
        ServerConfig::getInstance()->save();
        write_txbuf("Saved!\n");
      } else if (args[0] == "exit") {
        req_quit_ = true;
        write_txbuf("Bye!\n");
      } else if (args[0] == "ls") {
        string titles = Snippets::getInstance()->ls("  ");
        write_txbuf(titles);
        write_txbuf("\n");
      } else if (args[0] == "cat" && args.size() > 1) {
        string contents = Snippets::getInstance()->cat(args[1], "  ");
        write_txbuf(contents);
        write_txbuf("\n");
      } else if (args[0] == "ts") {
        bool t = ServerConfig::getInstance()->timestamp();
        ServerConfig::getInstance()->timestamp(!t);
        t = ServerConfig::getInstance()->timestamp();
        write_txbuf("Show timestamp: ");
        write_txbuf(t ? BOLD_GREEN "ON" NONE :
                        BOLD_RED "OFF" NONE);
        write_txbuf("\n");
      } else {
        if (Snippets::getInstance()->exists(args[0])) {
          string contents = Snippets::getInstance()->cat(args[0]);
          transfer(contents.c_str(), contents.length());
          transfer("\n", 1);
        } else {
          write_txbuf(BOLD_RED);
          write_txbuf(args[0]);
          write_txbuf(": ");
          write_txbuf("command not found!\n" NONE);
          save_history = false;
        }
      }
      if (save_history)
        history_.push_back(cmd_input);
    }
    if (!req_quit_) {
      write_txbuf(MGR_PROMPT);
    }
    if (quit_)
      break;
  }
}

void AdminConnection::print_help() {
  string help_msg =
      "\n"
      "  connect    :  connect to serial port with the existing config\n"
      "  disconnect :  disconnect from serial port\n"
      "  reconnect  :  disconnect and then connect to serial port\n"
      "  set        :  set baudrate/databits/parity/stopbit [options]\n"
      "  save       :  save configuration\n"
      "  ts         :  enable/disable timestamp"
      "  ls         :  list snippets\n"
      "  cat <name> :  show contents of snippet\n"
      "  <cmd>      :  run snippet\n"
      "  exit       :  exit the current client connection\n"
      "  help       :  print this help message\n"
      "\n";
  write_txbuf(help_msg);
}
} // namespace SerialOverSocket
