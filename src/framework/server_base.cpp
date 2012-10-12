// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "server_base.hpp"

#include <fstream>
#include "../common/exception.hpp"
#include "mixable.hpp"
#include "mixer/mixer.hpp"

namespace {
void jubatus_serv::build_local_path(std::string& out,
                                     const std::string& type,
                                     const std::string& id) const {
  out = base_path_ + "/" + a_.eth + "_" + pfi::lang::lexical_cast<std::string>(a_.port) + "_";
  out += type + "_" + id + ".jc";
}


void jubatus_serv::build_local_path0(std::string& out,
                                     const std::string& type,
                                     const std::string& id) const {
  out = base_path_ + "/";
  out += type + "_" + id + ".jc";
}
}

namespace jubatus {
namespace framework {

server_base::server_base()
    : update_count_(0) {}

bool server_base::save(const std::string& path, const std::string& id) {
  std::ofstream ofs(path.c_str(), std::ios::trunc|std::ios::binary);
  if (!ofs) {
    throw JUBATUS_EXCEPTION(jubatus::exception::runtime_error(path + ": cannot open")
        << jubatus::exception::error_errno(errno));
  }
  try {
    std::vector<pfi::lang::shared_ptr<mixable0> > mixables = get_mixables();
    for (size_t i = 0; i < mixables.size(); ++i) {
      mixables[i]->save(ofs);
    }
    ofs.close();
    LOG(INFO) << "saved to " << path;
  } catch (const std::runtime_error& e) {
    LOG(ERROR) << e.what();
    throw;
  }
}

bool server_base::load(const std::string& id) {
  std::ifstream ifs(path.c_str(), std::ios::binary);
  if (!ifs) {
    throw JUBATUS_EXCEPTION(jubatus::exception::runtime_error(path + ": cannot open")
                            << jubatus::exception::error_errno(errno));
  }
  try {
    std::vector<pfi::lang::shared_ptr<mixable0> > mixables = get_mixables();
    for (size_t i = 0; i < mixables.size(); ++i) {
      mixables[i]->clear();
      mixables[i]->load(ifs);
    }
    ifs.close();
  } catch (const std::runtime_error& e) {
    ifs.close();
    LOG(ERROR) << e.what();
    throw;
  }
}

void server_base::event_model_updated() {
  ++update_count_;
  if (mixer::mixer* m = get_mixer()) {
    m->updated();
  }
}

}
}
