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
#include "server_util.hpp"

namespace jubatus {
namespace framework {

template<typename Server>
class server_helper {
public:
  typedef typename Server::status_t status_t;

  explicit server_helper(const server_argv& a)
      : a_(a),
#ifdef HAVE_ZOOKEEPER_H
        zk_(common::create_lock_service("zk", a.z, a.timeout, make_logfile_name())),
#endif
        server_(new Server(a, zk_)),
        idgen_(a_.is_standalone()),
        use_cht_(false) {}

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

  status_t get_status() const {
    std::map<std::string, std::string> data;
    util::get_machine_status(data);
  
    data["timeout"] = pfi::lang::lexical_cast<std::string>(a_.timeout);
    data["threadnum"] = pfi::lang::lexical_cast<std::string>(a_.threadnum);
    data["tmpdir"] = a_.tmpdir;
    data["interval_sec"] = pfi::lang::lexical_cast<std::string>(a_.interval_sec);
    data["interval_count"] = pfi::lang::lexical_cast<std::string>(a_.interval_count);
    data["is_standalone"] = pfi::lang::lexical_cast<std::string>(a_.is_standalone());
    data["VERSION"] = JUBATUS_VERSION;
    data["PROGNAME"] = a_.program_name;

    data["update_count"] = pfi::lang::lexical_cast<std::string>(server_->update_count());

    status_t status;

#ifdef HAVE_ZOOKEEPER_H
    server_->get_mixer()->get_status(status);
    data["zk"] = a_.z;
    data["use_cht"] = pfi::lang::lexical_cast<std::string>(use_cht_);
#endif
  
    status[get_server_identifier(a_)].swap(data);
    server_->get_status(status);

    return status;
  }

  void use_cht() {
    use_cht_ = true;
  }

  int start(pfi::network::mprpc::rpc_server& serv) {
#ifdef HAVE_ZOOKEEPER_H
    if (!a_.is_standalone()) {
      ls = zk_;
      common::prepare_jubatus(*zk_, a_.type, a_.name);
    
      std::string counter_path;
      common::build_actor_path(counter_path, a_.type, a_.name);
      idgen_.set_ls(zk_, counter_path);

      if (a_.join) { // join to the existing cluster with -j option
        LOG(INFO) << "joining to the cluseter " << a_.name;
        LOG(ERROR) << "join is not supported yet :(";
      }
    
      if (use_cht_) {
        jubatus::common::cht::setup_cht_dir(*zk_, a_.type, a_.name);
        jubatus::common::cht ht(zk_, a_.type, a_.name);
        ht.register_node(a_.eth, a_.port);
      }
    
      // FIXME(rethink): is this sequence correct?
      register_actor(*zk_, a_.type, a_.name, a_.eth, a_.port);
      server_->get_mixer()->start();
    }
#endif

    if (serv.serv(a_.port, a_.threadnum)) {
      LOG(INFO) << "running in port=" << a_.port;
      return 0;
    } else {
      LOG(ERROR) << "failed starting server: any process using port " << a_.port << "?";
      return -1;
    }
  }

  common::cshared_ptr<Server> server() const {
    return server_;
  }

  pfi::concurrent::rw_mutex& rw_mutex() {
    return rw_mutex_;
  }

private:
  std::string make_logfile_name() const {
    // FIXME: need to decide the log file
    // without log output file, zkclient outputs to stderr
    std::string logfile = "/tmp/";
    logfile += a_.program_name;
    logfile += ".";
    logfile += pfi::lang::lexical_cast<std::string>(getpid());
    logfile += ".zklog";
    return logfile;
  }
                                      
  const server_argv a_;
  common::cshared_ptr<jubatus::common::lock_service> zk_;
  pfi::concurrent::rw_mutex rw_mutex_;
  common::cshared_ptr<Server> server_;
  common::global_id_generator idgen_;
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
