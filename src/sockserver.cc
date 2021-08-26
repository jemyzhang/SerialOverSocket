//
// Created by jemyzhang on 16-11-14.
//

#include <cerrno>
#include <cstring>
#include <functional>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "ioloop.h"
#include "serialport.h"
#include "socket.h"
#include "sockserver.h"

namespace SerialOverSocket {

  int Server::close_connection(Connection *pcon) {
    if (pcon) {
      pcmgr_->remove(pcon);
      delete pcon;
    }
    return 0;
  }

  int Server::close_connection(int fd) {
    Connection *pcon = pcmgr_->find(fd);
    return close_connection(pcon);
  }

  void Server::close_all_connections(bool bexit) {
    Connection *pcon = nullptr;
    while ((pcon = pcmgr_->pop()) != nullptr) {
      close_connection(pcon);
    }
    close(server_socket_.fileno());
    close(admin_socket_.fileno());
    if (bexit) {
      exit(0);
    }
  }

  int Server::accept_connection(int srvfd) {
    struct sockaddr in_addr{};
    socklen_t in_len;
    int infd;
    while (true) {
      in_len = sizeof in_addr;
      infd = accept(srvfd, &in_addr, &in_len);
      if (infd == -1) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
          /* We have processed all incoming
             connections. */
          break;
        } else {
          perror("accept");
          break;
        }
      }

      char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
      if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,
                      NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        perror("getnameinfo");
      }

      /* Make the incoming socket non-blocking and add it to the
         list of fds to monitor. */
      Socket::setblocking(infd, false);
      IOLoop::getInstance().get()->addHandler(infd, this,
                                              EPOLLOUT | EPOLLIN | EPOLLET);

      Connection *pconn;
      if (srvfd == admin_socket_.fileno()) {
        pconn = new AdminConnection(infd, hbuf, sbuf);
      } else {
        pconn = new ClientConnection(infd, hbuf, sbuf);
      }
      pcmgr_->append(pconn);
    }
    return 0;
  }

  int Server::input_data_handler(int fd) {
    int done = 0;
    Connection *pconn = pcmgr_->find(fd);

    while (true) {
      ssize_t count, readbytes;
      char buf[128];
      if (pconn->type() == Connection::CONNECTION_SERIAL) {
        readbytes = 1;
      } else {
        readbytes = sizeof(buf);
      }

      count = read(fd, buf, readbytes);
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
        if (pconn) {
          if (pconn->type() == Connection::CONNECTION_SERIAL &&
              buf[0] == '\n' && ServerConfig::getInstance()->timestamp()) {
            struct tm *tm_info;
            struct timeval tv;
            char tsbuf[128];
            gettimeofday(&tv, NULL);
            tm_info = localtime(&tv.tv_sec);
            sprintf(tsbuf,
                    "\n\x1b[37;44m[%04d-%02d-%02d %02d:%02d:%02d.%03ld]\x1b[0m ",
                    (1900 + tm_info->tm_year), tm_info->tm_mon, tm_info->tm_mday,
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                    tv.tv_usec / 1000);
            pconn->write_rxbuf(tsbuf, strlen(tsbuf));
          } else {
            if (!SerialPort::getInstance()->isconnected()) {
              // try to reconnect
              SerialPort::getInstance()->connect();
            }
            pconn->write_rxbuf(buf, count);
          }
        }
      }
    }

    if (pconn->type() == Connection::CONNECTION_CLIENT) {
      /* received data from socket,
       * should be sent to the serial port */
      if (!SerialPort::getInstance()->isconnected()) {
        pconn->write_txbuf("Serial Port not connected\n");
      }
    }

    if (done) {
      close_connection(pconn);
    }

    return 0;
  }

  int Server::input_event_handler(int fd) {
    if (fd == server_socket_.fileno() || fd == admin_socket_.fileno()) {
      /* We have a notification on the listening socket, which
         means one or more incoming connections. */
      accept_connection(fd);
    } else {
      /* We have data on the fd_ waiting to be read. Read and
         display it. We must read whatever data is available
         completely, as we are running in edge-triggered mode
         and won't get a notification again for the same
         data. */
      input_data_handler(fd);
    }
    return 0;
  }

  int Server::output_data_handler(int fd) {
    Connection *pconn = pcmgr_->find(fd);
    if (pconn) {
      pconn->flush();
    }

    if (pconn && pconn->quiting()) {
      close_connection(pconn);
    }

    return 0;
  }

  int Server::handle(epoll_event e) {
    if ((e.events & EPOLLERR) || (e.events & EPOLLHUP) ||
        (!(e.events & EPOLLIN) && !(e.events & EPOLLOUT))) {
      /* An error has occured on this fd, or the socket is not
         ready for reading (why were we notified then?) */
      if (e.data.fd == SerialPort::getInstance()->fileno()) {
        SerialPort::getInstance()->disconnect();
      } else {
        IOLoop::getInstance().get()->removeHandler(e.data.fd);
      }
      close_connection(e.data.fd);
    } else if (e.events & EPOLLIN) {
      input_event_handler(e.data.fd);
    } else if (e.events & EPOLLOUT) {
      output_data_handler(e.data.fd);
    }
    return 0;
  }

  Server::Server()
     : cfg(ServerConfig::getInstance()),
       server_socket_(cfg->server_address(), cfg->server_port()),
       admin_socket_(cfg->server_address(), cfg->admin_port()) {

    server_socket_.setunblock();
    admin_socket_.setunblock();

    pcmgr_ = new ConnectionManager();

    IOLoop::getInstance()->addHandler(server_socket_.fileno(), this,
                                      EPOLLIN | EPOLLET);
    IOLoop::getInstance()->addHandler(admin_socket_.fileno(), this,
                                      EPOLLIN | EPOLLET);

    SerialPort::getInstance()->InstallStatusCallback(
       std::bind(&Server::OnSerialPortConnectionChanged, this, placeholders::_1,
                 placeholders::_2));

    SerialPort::getInstance()->connect(cfg);
  }

  Server::~Server() { close_all_connections(true); }

  void Server::OnSerialPortConnectionChanged(bool b, int fd) {
    if (b) {
      IOLoop::getInstance()->addHandler(fd, this, EPOLLIN | EPOLLET);
      pcmgr_->append(new SerialPortConnection(fd, cfg->serial_device(), ""));
    } else {
      IOLoop::getInstance().get()->removeHandler(fd);
      Connection *pcon = pcmgr_->remove(fd);
      if (pcon) {
        delete pcon;
      }
    }
  }
} // namespace SerialOverSocket
