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

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <string>
#include <glog/logging.h>
#include <pficommon/concurrent/rwmutex.h>
#include <pficommon/network/mprpc.h>
#include <pficommon/system/sysstat.h>
#include "../common/cht.hpp"
#include "../common/global_id_generator.hpp"
#include "../common/membership.hpp"
#include "../common/shared_ptr.hpp"
#include "../common/lock_service.hpp"
#include "mixer/mixer.hpp"
#include "server_util.hpp"

namespace jubatus {
namespace framework {

template<typename Server>
class server_helper {
public:
  typedef typename Server::status_t status_t;

  explicit server_helper(const server_argv& a)
      : use_cht_(false) {
#ifdef HAVE_ZOOKEEPER_H
    if (!a.is_standalone()) {
      zk_.reset(common::create_lock_service("zk", a.z, a.timeout, make_logfile_name()));
    }
#endif
    server_.reset(new Server(a, zk_));
  }

  std::map<std::string, std::string> get_loads() const {
    std::map<std::string, std::string> result;
    {
      pfi::system::sysstat::sysstat_ret sys;
      get_sysstat(sys);
      result["loadavg"] = pfi::lang::lexical_cast<std::string>(sys.loadavg);
      result["total_memory"] = pfi::lang::lexical_cast<std::string>(sys.total_memory);
      result["free_memory"] = pfi::lang::lexical_cast<std::string>(sys.free_memory);
    }
    return result;
  }

  std::map<std::string, status_t> get_status() const {
    std::map<std::string, status_t> status;
    const server_argv& a = server_->argv();
    status_t& data = status[get_server_identifier(a)];

    util::get_machine_status(data);
  
    data["timeout"] = pfi::lang::lexical_cast<std::string>(a.timeout);
    data["threadnum"] = pfi::lang::lexical_cast<std::string>(a.threadnum);
    data["tmpdir"] = a.tmpdir;
    data["interval_sec"] = pfi::lang::lexical_cast<std::string>(a.interval_sec);
    data["interval_count"] = pfi::lang::lexical_cast<std::string>(a.interval_count);
    data["is_standalone"] = pfi::lang::lexical_cast<std::string>(a.is_standalone());
    data["VERSION"] = JUBATUS_VERSION;
    data["PROGNAME"] = a.program_name;

    data["update_count"] = pfi::lang::lexical_cast<std::string>(server_->update_count());

    server_->get_status(data);

#ifdef HAVE_ZOOKEEPER_H
    server_->get_mixer()->get_status(data);
    data["zk"] = a.z;
    data["use_cht"] = pfi::lang::lexical_cast<std::string>(use_cht_);
#endif

    return status;
  }

  void use_cht() {
    use_cht_ = true;
  }

  int start(pfi::network::mprpc::rpc_server& serv) {
    const server_argv& a = server_->argv();
#ifdef HAVE_ZOOKEEPER_H
    if (!a.is_standalone()) {
      ls = zk_;
      common::prepare_jubatus(*zk_, a.type, a.name);
    
      if (a.join) { // join to the existing cluster with -j option
        LOG(INFO) << "joining to the cluseter " << a.name;
        LOG(ERROR) << "join is not supported yet :(";
      }
    
      if (use_cht_) {
        jubatus::common::cht::setup_cht_dir(*zk_, a.type, a.name);
        jubatus::common::cht ht(zk_, a.type, a.name);
        ht.register_node(a.eth, a.port);
      }
   
      // FIXME(rethink): is this sequence correct?
      register_actor(*zk_, a.type, a.name, a.eth, a.port);
      server_->get_mixer()->start();
    }
#endif

    if (serv.serv(a.port, a.threadnum)) {
      LOG(INFO) << "running in port=" << a.port;
      return 0;
    } else {
      LOG(ERROR) << "failed starting server: any process using port " << a.port << "?";
      return -1;
    }
  }

  common::cshared_ptr<Server> server() const {
    return server_;
  }

  pfi::concurrent::rw_mutex& rw_mutex() {
    return server_->rw_mutex();
  }

private:
  std::string make_logfile_name() const {
    // FIXME: need to decide the log file
    // without log output file, zkclient outputs to stderr
    std::string logfile = "/tmp/";
    logfile += server_->argv().program_name;
    logfile += ".";
    logfile += pfi::lang::lexical_cast<std::string>(getpid());
    logfile += ".zklog";
    return logfile;
  }
                                      
  common::cshared_ptr<jubatus::common::lock_service> zk_;
  common::cshared_ptr<Server> server_;
  bool use_cht_;
};

}
}

#define JRLOCK__(p) \
  ::pfi::concurrent::scoped_lock lk(::pfi::concurrent::rlock((p)->rw_mutex()))

#define JWLOCK__(p) \
  ::pfi::concurrent::scoped_lock lk(::pfi::concurrent::wlock((p)->rw_mutex())); \
  (p)->server()->event_model_updated()

#define NOLOCK__(p)
