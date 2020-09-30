//
// Created by jemyzhang on 20-1-21.
//

#include "snippets.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>

using namespace std;

namespace SerialOverSocket {
  shared_ptr<Snippets> Snippets::instance_ = nullptr;

  shared_ptr<Snippets> Snippets::getInstance() {
    if (!instance_) {
      instance_ = make_shared<Snippets>();
      struct passwd *pw = getpwuid(getuid());
      string loc = string(pw->pw_dir) + "/.config/sos/snippets";
      instance_->set_location(loc);
    }
    return instance_;
  }

  string Snippets::ls(string prefix) {
    if (location.empty()) {
      return "location of snippets not defined";
    }
    DIR *dir;
    struct dirent *ent;
    stringstream titles;
    if ((dir = opendir(location.c_str())) != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG) {
          titles << prefix << ent->d_name << endl;
        }
      }
      closedir(dir);
    } else {
      return "snippets dir not exists";
    }
    return titles.str();
  }

  bool Snippets::exists(string title) {
    if (location.empty() || title.empty()) {
      return false;
    }
    string snip = location + "/" + title;
    return access(snip.c_str(), F_OK) != -1;
  }

  string Snippets::cat(string title, string prefix) {
    if (!exists(title)) {
      return "snippet not found";
    }
    string snip = location + "/" + title;
    ifstream fsnip(snip);
    if (!fsnip.is_open()) {
      return "snippet could not be opened";
    }
    string l;
    stringstream contents;
    while (getline(fsnip, l)) {
      contents << prefix << l << endl;
    }
    fsnip.close();
    return contents.str();
  }

} // namespace SerialOverSocket
