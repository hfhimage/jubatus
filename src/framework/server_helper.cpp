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

#include "server_helper.hpp"

#include "../common/cht.hpp"
#include "../common/membership.hpp"

using namespace std;
using namespace jubatus::common;

namespace jubatus {
namespace framework {

namespace {

string make_logfile_name(const server_argv& a) {
  // FIXME: need to decide the log file
  // without log output file, zkclient outputs to stderr
  std::string logfile = "/tmp/";
  logfile += a.program_name;
  logfile += ".";
  logfile += pfi::lang::lexical_cast<std::string>(getpid());
  logfile += ".zklog";
  return logfile;
}

}

server_helper_impl::server_helper_impl(const server_argv& a) {
#ifdef HAVE_ZOOKEEPER_H
  if (!a.is_standalone()) {
    zk_.reset(common::create_lock_service("zk", a.z, a.timeout, make_logfile_name(a)));
  }
#endif
}

bool server_helper_impl::prepare_for_start(const server_argv& a, bool use_cht) {
#ifdef HAVE_ZOOKEEPER_H
  if (!a.is_standalone()) {
    ls = zk_;
    common::prepare_jubatus(*zk_, a.type, a.name);
    
    if (a.join) { // join to the existing cluster with -j option
      LOG(INFO) << "joining to the cluseter " << a.name;
      LOG(ERROR) << "join is not supported yet :(";
    }
    
    if (use_cht) {
      jubatus::common::cht::setup_cht_dir(*zk_, a.type, a.name);
      jubatus::common::cht ht(zk_, a.type, a.name);
      ht.register_node(a.eth, a.port);
    }
   
    // FIXME(rethink): is this sequence correct?
    register_actor(*zk_, a.type, a.name, a.eth, a.port);
    return true;
  }
#endif
  return false;
}

}
}
