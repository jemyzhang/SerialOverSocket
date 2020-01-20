//
// Created by jemyzhang on 17-6-13.
//

#include "serialport.h"
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace SerialOverSocket {
namespace {
struct {
  int baudrate;
  speed_t speed;
} baudrate_pairs[] = {
    {115200, B115200}, {57600, B57600}, {38400, B38400},
    {19200, B19200},   {9600, B9600},   {4800, B4800},
    {2400, B2400},     {1200, B1200},   {300, B300},
};
}
shared_ptr<SerialPort> SerialPort::instance_ = nullptr;

shared_ptr<SerialPort> SerialPort::getInstance() {
  if (!instance_) {
    instance_ = make_shared<SerialPort>();
  }
  return instance_;
}

SerialPort::SerialPort() : fd_(-1) { callback_.clear(); }

SerialPort::~SerialPort() {
  disconnect();
  callback_.clear();
}

int SerialPort::connect(shared_ptr<ServerConfig> cfg) {
  if (fd_ > 0)
    return fd_;

  this->cfg_ = cfg;

  int fd;
  int i;
  struct termios options {};

  if ((fd = open(cfg->serial_device().c_str(), O_RDWR | O_NONBLOCK)) <= 0) {
    perror("serial port open");
    return -1;
  }

  try {
    if (tcgetattr(fd, &options) != 0) {
      throw("failed to get tc attr");
    }

    // set to raw mode
    cfmakeraw(&options);

    bool baudrate_valid = false;
    for (i = 0; i < sizeof(baudrate_pairs) / sizeof(baudrate_pairs[0]); i++) {
      if (cfg->serial_baudrate() == baudrate_pairs[i].baudrate) {
        cfsetispeed(&options, baudrate_pairs[i].speed);
        cfsetospeed(&options, baudrate_pairs[i].speed);
        baudrate_valid = true;
        break;
      }
    }
    if (!baudrate_valid) {
      throw("Unsupported baudrate");
    }

    options.c_cflag &= ~CSIZE;
    switch (cfg->serial_databits()) {
    case 7:
      options.c_cflag |= CS7;
      break;
    case 8:
      options.c_cflag |= CS8;
      break;
    default:
      throw("Unsupported data bits");
    }
    switch (cfg->serial_parity()) {
    case 'n':
    case 'N':
      options.c_cflag &= ~PARENB; /* Clear parity enable */
      options.c_iflag &= ~INPCK;  /* Enable parity checking */
      break;
    case 'o':
    case 'O':
      options.c_cflag |= (PARODD | PARENB);
      options.c_iflag |= INPCK; /* Disable parity checking */
      break;
    case 'e':
    case 'E':
      options.c_cflag |= PARENB; /* Enable parity */
      options.c_cflag &= ~PARODD;
      options.c_iflag |= INPCK; /* Disable parity checking */
      break;
    case 'S':
    case 's': /*as no parity*/
      options.c_cflag &= ~PARENB;
      options.c_cflag &= ~CSTOPB;
      break;
    default:
      throw("Unsupported parity");
    }
    /* Set input parity option */
    if (cfg->serial_parity() != 'n') {
      options.c_iflag |= INPCK;
    }
    /* Set stopbit*/
    switch (cfg->serial_stopbit()) {
    case 1:
      options.c_cflag &= ~CSTOPB;
      break;
    case 2:
      options.c_cflag |= CSTOPB;
      break;
    default:
      throw("Unsupported stopbit");
    }

    tcflush(fd, TCIOFLUSH);
    tcflush(fd, TCIFLUSH);

    options.c_cc[VTIME] = 150; /* set timeout: 15 seconds*/
    options.c_cc[VMIN] = 0;    /* Update the options and do it NOW */

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
      throw("failed setup serial port");
    }

    fd_ = fd;

    for (auto &c : callback_) {
      c(true, fd);
    }

    return fd;
  } catch (const char *str) {
    cerr << str << endl;
    if (fd > 0) {
      close(fd);
    }
    return -1;
  }
}

void SerialPort::disconnect() {
  if (fd_ > 0) {
    for (auto &c : callback_) {
      c(false, fd_);
    }
    close(fd_);
    fd_ = -1;
  }
}

int SerialPort::reconnect() {
  disconnect();
  connect();
  return 0;
}
}
