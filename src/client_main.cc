//
// Created by jemyzhang on 16-12-30.
//

#include "config.h"
#include "ioloop.h"
#include "sockclient.h"
#include <csignal>
#include <getopt.h>
#include <memory>

#define SOS_CLIENT_VER "1.0.0"
static const char *optstr = "p:a:c:hv";
static const struct option opts[] = {{"", required_argument, nullptr, 'c'},
                                     {"", no_argument, nullptr, 'h'},
                                     {"", no_argument, nullptr, 'v'},
                                     {nullptr, no_argument, nullptr, 0}};

static void print_version() {
  printf("Serial Over Socket Client\n");
  printf("version: %s\n", SOS_CLIENT_VER);
}
static void print_help(const char *prog) {
  printf("Usage: %s -p [port] [-hlv]\n\n", prog);
  printf("Options:\n");
  printf("    -c config      : path to the config file\n");
  printf("    -v             : show version info\n");
  printf("    -h             : print this help\n");
}

static void parse_options(int argc, char *argv[]) {
  int opt = 0, idx = 0;
  string conf_path = string();

  while ((opt = getopt_long(argc, argv, optstr, opts, &idx)) != -1) {
    switch (opt) {
    case 'v':
      print_version();
      exit(EXIT_SUCCESS);
    case 'h':
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    case 'c':
      conf_path = optarg;
      break;
    case '?':
      printf("Unrecognized option: %s\n", optarg);
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if(!SerialOverSocket::Config::getInstance()->load_config_file(conf_path)) {
    exit(1);
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
  parse_options(argc, argv);

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGPIPE, sig_handler);
  signal(SIGUSR1, sig_handler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGABRT, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  // signal(SIGUSR1, SIG_IGN);
  SerialOverSocket::Client client;
  SerialOverSocket::IOLoop::getInstance().get()->start();
  return 0;
}
