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
    {4000000, B4000000}, {3500000, B3500000}, {3000000, B3000000},
    {2500000, B2500000}, {2000000, B2000000}, {1500000, B1500000},
    {1152000, B1152000}, {1000000, B1000000}, {921600, B921600},
    {576000, B576000},   {500000, B500000},   {460800, B460800},
    {230400, B230400},   {115200, B115200},   {57600, B57600},
    {38400, B38400},     {19200, B19200},     {9600, B9600},
    {4800, B4800},       {2400, B2400},       {1200, B1200},
    {300, B300},
};
} // namespace
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

bool SerialPort::set_baudrate(int baudrate, bool instantly) {
  if (fd_ < 0)
    return false;
  for (int i = 0; i < sizeof(baudrate_pairs) / sizeof(baudrate_pairs[0]); i++) {
    if (baudrate == baudrate_pairs[i].baudrate) {
      cfsetispeed(&options_, baudrate_pairs[i].speed);
      cfsetospeed(&options_, baudrate_pairs[i].speed);
      return instantly ? tcsetattr(fd_, TCSANOW, &options_) == 0 : true;
    }
  }
  return false;
}

bool SerialPort::set_databits(int databits, bool instantly) {
  if (fd_ < 0)
    return false;
  switch (databits) {
  case 7:
    options_.c_cflag |= CS7;
    break;
  case 8:
    options_.c_cflag |= CS8;
    break;
  default:
    return false;
  }
  return instantly ? tcsetattr(fd_, TCSANOW, &options_) == 0 : true;
}

bool SerialPort::set_parity(char parity, bool instantly) {
  if (fd_ < 0)
    return false;
  switch (parity) {
  case 'n':
  case 'N':
    options_.c_cflag &= ~PARENB; /* Clear parity enable */
    options_.c_iflag &= ~INPCK;  /* Enable parity checking */
    break;
  case 'o':
  case 'O':
    options_.c_cflag |= (PARODD | PARENB);
    options_.c_iflag |= INPCK; /* Disable parity checking */
    break;
  case 'e':
  case 'E':
    options_.c_cflag |= PARENB; /* Enable parity */
    options_.c_cflag &= ~PARODD;
    options_.c_iflag |= INPCK; /* Disable parity checking */
    break;
  case 'S':
  case 's': /*as no parity*/
    options_.c_cflag &= ~PARENB;
    options_.c_cflag &= ~CSTOPB;
    break;
  default:
    return false;
  }
  /* Set input parity option */
  if (parity != 'n') {
    options_.c_iflag |= INPCK;
  }
  return instantly ? tcsetattr(fd_, TCSANOW, &options_) == 0 : true;
}

bool SerialPort::set_stopbit(char stopbit, bool instantly) {
  if (fd_ < 0)
    return false;
  /* Set stopbit*/
  switch (stopbit) {
  case 1:
    options_.c_cflag &= ~CSTOPB;
    break;
  case 2:
    options_.c_cflag |= CSTOPB;
    break;
  default:
    return false;
  }
  return instantly ? tcsetattr(fd_, TCSANOW, &options_) == 0 : true;
}

int SerialPort::connect(shared_ptr<ServerConfig> cfg) {
  if (fd_ > 0)
    return fd_;

  this->cfg_ = cfg;

  int fd;
  int i;

  if ((fd = open(cfg->serial_device().c_str(), O_RDWR | O_NONBLOCK)) <= 0) {
    perror("serial port open");
    return -1;
  }

  fd_ = fd;

  try {
    if (tcgetattr(fd, &options_) != 0) {
      throw("failed to get tc attr");
    }

    // set to raw mode
    cfmakeraw(&options_);

    if (!set_baudrate(cfg->serial_baudrate())) {
      throw("Unsupported baudrate");
    }

    options_.c_cflag &= ~CSIZE;
    if (!set_databits(cfg->serial_databits())) {
      throw("Unsupported data bits");
    }

    if (!set_parity(cfg->serial_parity())) {
      throw("Unsupported parity");
    }
    /* Set stopbit*/
    if (!set_stopbit(cfg->serial_stopbit())) {
      throw("Unsupported stopbit");
    }

    tcflush(fd, TCIOFLUSH);
    tcflush(fd, TCIFLUSH);

    options_.c_cc[VTIME] = 150; /* set timeout: 15 seconds*/
    options_.c_cc[VMIN] = 0;    /* Update the options and do it NOW */

    if (tcsetattr(fd, TCSANOW, &options_) != 0) {
      throw("failed setup serial port");
    }

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
} // namespace SerialOverSocket
