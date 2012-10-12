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

#include "linear_mixer.hpp"

#include <glog/logging.h>
#include <pficommon/concurrent/lock.h>
#include <pficommon/concurrent/rwmutex.h>
#include <pficommon/lang/bind.h>
#include <pficommon/system/time_util.h>
#include "../../common/exception.hpp"
#include "../../common/membership.hpp"
#include "../../common/mprpc/rpc_client.hpp"
#include "../mixable.hpp"

using namespace std;
using namespace pfi::concurrent;

namespace jubatus {
namespace framework {
namespace mixer {

linear_mixer::linear_mixer(common::cshared_ptr<common::lock_service>& zk,
                           const string& type, const string& name,
                           int timeout_sec,
                           unsigned int count_threshold, unsigned int tick_threshold)
    : zk_(zk),
      type_(type),
      name_(name),
      timeout_sec_(timeout_sec),
      count_threshold_(count_threshold),
      tick_threshold_(tick_threshold),
      counter_(0),
      ticktime_(0),
      mix_count_(0),
      is_running_(false),
      t_(pfi::lang::bind(&linear_mixer::mixer_loop, this)) {}

void linear_mixer::register_api(pfi::network::mprpc::rpc_server& server) {
  server.add<std::vector<std::string>(int)>
      ("get_diff",
       pfi::lang::bind(&linear_mixer::get_diff, this, pfi::lang::_1));
  server.add<int(std::vector<std::string>)>
      ("put_diff",
       pfi::lang::bind(&linear_mixer::put_diff, this, pfi::lang::_1));
}

void linear_mixer::register_mixable(const pfi::lang::shared_ptr<mixable0>& m) {
  mixables_.push_back(m);
}

void linear_mixer::start() {
  scoped_lock lk(wlock(m_));
  if (!is_running_) {
    t_.start();
    is_running_ = true;
  }
}

void linear_mixer::stop() {
  scoped_lock lk(wlock(m_));
  if (is_running_) {
    is_running_ = false;
    t_.join();
  }
}

void linear_mixer::updated() {
  scoped_lock lk(wlock(m_));
  unsigned int new_ticktime = time(NULL);
  ++counter_;
  if(counter_ > count_threshold_ ||
     new_ticktime - ticktime_ > tick_threshold_){
    c_.notify(); // FIXME: need sync here?
  }
  return counter_;
}

void linear_mixer::mixer_loop() {
  while (is_running_) {
    string path;
    common::build_actor_path(path, type_, name_);
    common::lock_service_mutex zklock(*zk_, path + "/master_lock");
    try {
      {
        scoped_lock lk(wlock(m_));

        c_.wait(m_, 1);
        unsigned int new_ticktime = time(NULL);
        if (counter_ > count_threshold_ || new_ticktime - ticktime_ > tick_threshold_) {
          if (zklock.try_lock()) {
            DLOG(INFO) << "starting mix:";
            counter_ = 0;
            ticktime_ = new_ticktime;
          } else {
            return;
          }
        } else {
          return;
        }

      } //unlock
      DLOG(INFO) << ".... " << mix_count_ << "th mix done.";

    } catch (const jubatus::exception::jubatus_exception& e) {
      LOG(ERROR) << e.diagnostic_information(true);
    }
  }
}

void linear_mixer::mix() {
  using namespace pfi::system::time;

  vector<pair<string,int> > servers;
  common::get_all_actors(*zk_, type_, name_, servers);
  vector<string> accs;
  //vector<string> serialized_diffs;
  clock_time start = get_clock_time();

  if (servers.empty()) {
    LOG(WARNING) << "no other server. ";
    return;
  } else {
    common::mprpc::rpc_mclient c(servers, timeout_sec_);
    try {
      common::mprpc::rpc_result_object result = c.call("get_diff", 0);

      vector<string> mixed = result.response.front().as<vector<string> >();
      for (size_t i = 1; i < result.response.size(); ++i) {
        vector<string> tmp = result.response[i].as<vector<string> >();
        for (size_t j = 0; j < tmp.size(); ++j) {
          mixables_[i]->reduce(tmp[j], mixed[j]);
        }
      }

      c.call("put_diff", mixed);
      // TODO: output log when result has error
    } catch (const std::exception& e) {
      LOG(WARNING) << e.what() << " : mix failed";
      return;
    }
  }

  clock_time end = get_clock_time();
  size_t s = 0;
  for (size_t i = 0; i < accs.size(); ++i) {
    s += accs[i].size();
  }
  DLOG(INFO) << "mixed with " << servers.size() << " servers in " << (double)(end - start) << " secs, "
             << s << " bytes (serialized data) has been put.";
  mix_count_++;
}

vector<string> linear_mixer::get_diff(int) {
  std::vector<std::string> o;

  scoped_lock lk(rlock(m_));
  if (mixables_.empty()) {
    throw JUBATUS_EXCEPTION(config_not_set()); // nothing to mix
  }
  for (size_t i = 0; i < mixables_.size(); ++i) {
    o.push_back(mixables_[i]->get_diff());
  }
  return o;
}

int linear_mixer::put_diff(const std::vector<std::string>& unpacked) {
  scoped_lock lk(wlock(m_));
  if (unpacked.size() != mixables_.size()) {
    //deserialization error
    return -1;
  }
  for (size_t i = 0; i < mixables_.size(); ++i) {
    mixables_[i]->put_diff(unpacked[i]);
  }
  counter_ = 0;
  ticktime_ = time(NULL);
  return 0;
}

}
}
}
