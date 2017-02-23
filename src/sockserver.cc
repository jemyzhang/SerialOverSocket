//
// Created by jemyzhang on 16-11-14.
//

#include <cerrno>
#include <cstring>
#include <functional>
#include <iostream>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

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
  close(manager_socket_.fileno());
  if (bexit) {
    exit(0);
  }
}

int Server::accept_connection(int srvfd) {
  struct sockaddr in_addr {};
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
    if (srvfd == manager_socket_.fileno()) {
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
      if (pconn) {
        pconn->write_rxbuf(buf, count);
      }
    }
  }

  if (pconn->type() == Connection::CONNECTION_CLIENT) {
    /* received data from socket,
     * should be send to the serial port */
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
  if (fd == server_socket_.fileno() || fd == manager_socket_.fileno()) {
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

Server::Server(Config::Info &info)
    : server_socket_(info.server.address, info.server.port),
      manager_socket_(info.server.address, info.server.manager.port) {

  server_socket_.setunblock();
  manager_socket_.setunblock();

  pcmgr_ = new ConnectionManager();

  IOLoop::getInstance()->addHandler(server_socket_.fileno(), this,
                                    EPOLLIN | EPOLLET);
  IOLoop::getInstance()->addHandler(manager_socket_.fileno(), this,
                                    EPOLLIN | EPOLLET);

  SerialPort::getInstance()->InstallStatusCallback(
      std::bind(&Server::OnSerialPortConnectionChanged, this, placeholders::_1,
                placeholders::_2));

  SerialPort::getInstance()->connect(info.serial);
}

Server::~Server() { close_all_connections(true); }

void Server::OnSerialPortConnectionChanged(bool b, int fd) {
  if (b) {
    IOLoop::getInstance()->addHandler(fd, this, EPOLLIN | EPOLLET);
    Config::SerialPort cfg = SerialPort::getInstance()->getConfig();
    pcmgr_->append(new SerialPortConnection(fd, cfg.devname, ""));
  } else {
    IOLoop::getInstance().get()->removeHandler(fd);
    Connection *pcon = pcmgr_->remove(fd);
    if (pcon) {
      delete pcon;
    }
  }
}
}
