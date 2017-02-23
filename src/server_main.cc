//
// Created by jemyzhang on 16-12-30.
//

#include "ioloop.h"
#include "sockserver.h"
#include <csignal>
#include <getopt.h>

#define DEFAULT_CONFIG_PATH "/etc/serialoversock/config.json"

#define SOS_SERVER_VER "1.0.0"
static const char *optstr = "p:a:c:b:d:s:r:t:hv";
static const struct option opts[] = {{"", required_argument, nullptr, 'p'},
                                     {"", required_argument, nullptr, 'a'},
                                     {"", required_argument, nullptr, 'c'},
                                     {"", required_argument, nullptr, 'b'},
                                     {"", required_argument, nullptr, 'd'},
                                     {"", required_argument, nullptr, 's'},
                                     {"", required_argument, nullptr, 'r'},
                                     {"", required_argument, nullptr, 't'},
                                     {"", no_argument, nullptr, 'h'},
                                     {"", no_argument, nullptr, 'v'},
                                     {nullptr, no_argument, nullptr, 0}};

static void print_version() {
  printf("Serial Over Socket Server\n");
  printf("version: %s\n", SOS_SERVER_VER);
}
static void print_help(const char *prog) {
  printf("Usage: %s -p [port] [-hlv]\n\n", prog);
  printf("Options:\n");
  printf("    -p port        : server port, default: 10303\n");
  printf("    -a address     : server address, default: 0.0.0.0\n");
  printf("    -c config      : path to the config file\n");
  printf("    -t device      : serial device name\n");
  printf("    -b baudrate    : baud-rate\n");
  printf("    -d databits    : databits\n");
  printf("    -s stopbit     : stopbit\n");
  printf("    -r parity      : parity\n");
  printf("    -v             : show version info\n");
  printf("    -h             : print this help\n");
}

static void parse_options(int argc, char *argv[],
                          SerialOverSocket::Config *pconf) {
  int opt = 0, idx = 0;
  const char *conf_path = nullptr;

  while ((opt = getopt_long(argc, argv, optstr, opts, &idx)) != -1) {
    switch (opt) {
    case 'v':
      print_version();
      exit(EXIT_SUCCESS);
    case 'h':
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    case 'a':
      pconf->detail_.server.address = optarg;
      break;
    case 'p':
      pconf->detail_.server.port = atoi(optarg);
      break;
    case 'c':
      conf_path = optarg;
      break;
    case 't':
      pconf->detail_.serial.devname = optarg;
      break;
    case 'b':
      pconf->detail_.serial.baudrate = atoi(optarg);
      break;
    case 'd':
      pconf->detail_.serial.databits = atoi(optarg);
      break;
    case 's':
      pconf->detail_.serial.stopbit = atoi(optarg);
      break;
    case 'r':
      pconf->detail_.serial.parity = optarg[0];
      break;
    case '?':
      printf("Unrecognized option: %s\n", optarg);
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if (argc == 1 && conf_path == nullptr) {
    conf_path = DEFAULT_CONFIG_PATH;
  }
  if (conf_path != nullptr) {
    if (!pconf->load_config_file(conf_path)) {
      exit(1);
    }
  }
}

static void sig_handler(int sig) {
  switch (sig) {
  case SIGINT:
  case SIGTERM:
    printf("sig term/int\n");
    exit(0);
    break;
  case SIGPIPE:
    printf("sigpipe received\n");
    break;
  default:
    printf("signal %d\n", sig);
    break;
  }
}

int main(int argc, char *argv[]) {
  SerialOverSocket::Config cfg;
  parse_options(argc, argv, &cfg);

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGPIPE, sig_handler);
  signal(SIGUSR1, sig_handler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGABRT, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  // signal(SIGUSR1, SIG_IGN);
  SerialOverSocket::Server server(cfg.detail_);
  SerialOverSocket::IOLoop::getInstance().get()->start();
  return 0;
}
