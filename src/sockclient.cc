//
// Created by jemyzhang on 16-11-14.
//

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>

#include "ansi_color.h"
#include "ioloop.h"
#include "snippets.h"
#include "sockclient.h"

namespace SerialOverSocket {

#define MGR_PROMPT_STR "MGR$ "
#define MGR_PROMPT YELLOW MGR_PROMPT_STR NONE

void Client::print_help() {
  string help_msg = "[local commands]\n"
                    "  ls         :  list snippets\n"
                    "  cat <name> :  show contents of snippet\n"
                    "  exit       :  exit the current client connection\n"
                    "  <cmd>      :  run snippet\n"
                    "  log        : show current log status\n"
                    "      start <file> : start log to file\n"
                    "      stop         : stop log without file parameter\n"
                    "  help       :  print this help message\n";
  cout << help_msg;
}

void Client::start_log(string path) {
  if (log_fd_ > 0) {
    cout << "log file switch to " << path << endl;
    int fd = log_fd_;
    log_fd_ = -1;
    close(fd);
  }
  int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if (fd == -1) {
    cout << "error start log, failed to open file " << path << strerror(errno)
         << endl;
    return;
  }
  log_fd_ = fd;
  log_file_ = path;
  cout << "log started" << endl;
}

void Client::stop_log() {
  if (log_fd_ > 0) {
    int fd = log_fd_;
    log_fd_ = -1;
    close(fd);
    log_file_ = "";
    cout << "log stopped" << endl;
  } else {
    cout << "log not started, start log with `log /path/to/file`" << endl;
  }
};

void Client::handle_cmd(string cmd) {
  std::stringstream ss(cmd);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> args(begin, end);
  bool save_history = true;
  if (args[0] == "help") {
    write_txbuf(cmd);
    print_help();
  } else if (args[0] == "history") {
    for (auto &s : history_) {
      cout << s << endl;
    }
  } else if (args[0] == "exit") {
    req_quit_ = true;
    cout << "exiting ... " << endl;
    write_txbuf(cmd);
  } else if (args[0] == "ls") {
    string titles = Snippets::getInstance()->ls("  ");
    cout << titles << endl;
  } else if (args[0] == "cat" && args.size() > 1) {
    string contents = Snippets::getInstance()->cat(args[1], "  ");
    cout << contents << endl;
  } else if (args[0] == "log") {
    if (args.size() > 1) {
      if (args[1] == "start" && args.size() > 2) {
        start_log(args[2]);
      } else if (args[1] == "stop") {
        stop_log();
      }
    } else {
      if (log_fd_ > 0) {
        cout << "logging to file: " << log_file_ << endl;
      } else {
        cout << "log not started" << endl;
      }
    }
  } else {
    if (Snippets::getInstance()->exists(args[0])) {
      // disconnect from control panel and run snippets
      switch_admin_connection(false);
      string contents = Snippets::getInstance()->cat(args[0]);
      write_txbuf(contents);
      write_txbuf("\n", 1);
    } else {
      write_txbuf(cmd);
    }
  }
  if (save_history)
    history_.push_back(cmd);
}

void Client::process_console_input(const char *content, ssize_t length) {
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

        cout << CLRLINE MGR_PROMPT;
        cmdline_.clear();
        cursor_pos_ = 0;
        if (content[2] == 'A') {
          history_idx_++;
          if (history_idx_ > len_his) {
            history_idx_ = len_his;
          }
        } else {
          history_idx_--;
          if (history_idx_ < 0) {
            history_idx_ = 0;
          }
        }
        if (history_idx_ > 0) {
          cmdline_ = history_.at(len_his - history_idx_);
          cout << cmdline_;
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
      cout << endl;
      if (!cmdline_.empty())
        handle_cmd(cmdline_);
      cmdline_.clear();
      cursor_pos_ = 0;
      history_idx_ = 0;
      // ask server to reply with MGR PROMPT
      write_txbuf("\n", 1);
      return;
    case 0x3:
      echo = false;
      ignore = true;
      cmdline_.clear();
      cursor_pos_ = 0;
      cout << endl << MGR_PROMPT;
      break;
    case 0x7f:
      ignore = true;
      echo = false;
      if (!cmdline_.empty() && cursor_pos_ > 0) {
        cursor_pos_--;
        cmdline_.erase(cursor_pos_, 1);
        cout << MOVE_LEFT(1) << CLRAFTER;
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

  char s[32];
  sprintf(s, "\r\e[%ldC", cursor_pos_ + strlen(MGR_PROMPT_STR));
  string output = CLRLINE MGR_PROMPT;
  output += cmdline_;
  output += s;
  write(fileno(stdout), output.c_str(), output.length());
}

int Client::input_data_handler(int fd) {
  int done = 0;
  while (true) {
    ssize_t count;
    char buf[128];
    memset(buf, 0, sizeof(buf));

    count = read(fd, buf, sizeof buf);
    if (count == -1) {
      /* If errno == EAGAIN, that means we have read all
         data. So go back to the main loop. */
      if (errno != EAGAIN) {
        perror("read");
        done = 1;
      }
      break;
    } else if (count == 0) {
      /* End of file. The remote has closed the
         connection. */
      done = 1;
      break;
    } else {
      if (fd == fileno(stdin)) {
        if (count == 1 && buf[0] == 0x11) {
          // Ctrl+Q
          if (!admin_socket_) {
            cout << endl
                 << YELLOW "Connecting to control panel..." NONE << endl;
          } else {
            cout << endl
                 << YELLOW "Disconnecting from control panel..." NONE << endl;
          }
          switch_admin_connection(!admin_socket_);
        } else {
          if (admin_socket_) {
            process_console_input(buf, count);
          } else {
            write_txbuf(buf, count);
          }
        }
      } else {
        if (fd == client_fd_ && admin_socket_) {
          // store buffer while in admin mode
          client_buf.append(buf, count);
          continue;
        }
        write_rxbuf(buf, count);
      }
    }
  }

  if (done) {
    cerr << "Server Connection Closed" << endl;
    exit(0);
  }

  return 0;
}

int Client::input_event_handler(int fd) {
  /* We have data on the fd_ waiting to be read. Read and
     display it. We must read whatever data is available
     completely, as we are running in edge-triggered mode
     and won't get a notification again for the same
     data. */
  input_data_handler(fd);
  return 0;
}

int Client::output_data_handler() {
  flush();

  return 0;
}

int Client::handle(epoll_event e) {
  if ((e.events & EPOLLERR) || (e.events & EPOLLHUP) ||
      (!(e.events & EPOLLIN) && !(e.events & EPOLLOUT))) {
    /* An error has occured on this fd, or the socket is not
       ready for reading (why were we notified then?) */
    cerr << "epoll error" << endl;
    IOLoop::getInstance().get()->removeHandler(e.data.fd);
    close(e.data.fd);
    exit(0);
  } else if (e.events & EPOLLIN) {
    input_event_handler(e.data.fd);
  } else if (e.events & EPOLLOUT) {
    output_data_handler();
  }
  return 0;
}

Client::Client()
    : Connection(0, {Connection::CONNECTION_CLIENT, "", ""}),
      cfg(ClientConfig::getInstance()),
      client_socket_(cfg->server_address(), cfg->server_port(), true) {

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

  client_buf.clear();
  if (client_socket_.fileno() < 0) {
    cerr << "failed to connect to the server: " << cfg->server_address() << ":"
         << cfg->server_port() << endl;
    exit(1);
  }

  log_fd_ = -1;
  client_fd_ = fd_ = client_socket_.fileno();
  client_socket_.setunblock();

  set_termio_mode();
  Socket::setunblock(fileno(stdin));

  IOLoop::getInstance()->addHandler(client_socket_.fileno(), this,
                                    EPOLLIN | EPOLLET);
  IOLoop::getInstance()->addHandler(fileno(stdin), this, EPOLLIN | EPOLLET);
}

Client::~Client() {
  if (log_fd_ > 0) {
    int fd = log_fd_;
    log_fd_ = -1;
    close(fd);
  }
  if (!history_.empty()) {
    // save history
    string histfile =
        string(getpwuid(getuid())->pw_dir) + "/.config/sos/history";
    ofstream of(histfile);
    ostream_iterator<string> oiter(of, "\n");
    copy(history_.begin(), history_.end(), oiter);
  }
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_option_);
}

ssize_t Client::write_rxbuf(const char *content, ssize_t length) {
  if (log_fd_ > 0) {
    write(log_fd_, content, length);
  }
  return write(fileno(stdout), content, length);
}

void Client::set_termio_mode() {
  struct termios options {};
  if (tcgetattr(fileno(stdin), &options) != 0) {
    throw("failed to get tc attr");
  }

  term_option_ = options;

  cfmakeraw(&options);
  options.c_iflag |= (ICRNL);
  options.c_oflag |= (OPOST | ONLCR);

  options.c_lflag &= ~ECHO;
  options.c_lflag &= ~ICANON;
  options.c_cc[VMIN] = 1;
  options.c_cc[VTIME] = 1;
  tcsetattr(fileno(stdin), TCSAFLUSH, &options);
}

void Client::switch_admin_connection(bool connect) {
  if (connect) {
    if (admin_socket_) {
      return;
    } else {
      admin_socket_ = std::make_unique<Socket>(cfg->server_address(),
                                               cfg->admin_port(), true);
      if (admin_socket_->fileno()) {
        admin_socket_->setunblock();
        client_fd_ = fd_;
        fd_ = admin_socket_->fileno();
        IOLoop::getInstance()->addHandler(fd_, this, EPOLLIN | EPOLLET);
      }
    }
  } else {
    if (admin_socket_) {
      IOLoop::getInstance()->removeHandler(fd_);
      fd_ = client_fd_;
      admin_socket_.reset();
      if (!client_buf.empty()) {
        // flush out the buffer
        write_rxbuf(client_buf.c_str(), client_buf.size());
        client_buf.clear();
      }
    }
  }
}
} // namespace SerialOverSocket
