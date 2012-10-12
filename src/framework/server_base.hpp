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

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <pficommon/lang/shared_ptr.h>

namespace jubatus {
namespace framework {

class mixable0;

namespace mixer {
class mixer;
}

class server_base {
public:
  typedef std::map<std::string, std::map<std::string, std::string> > status_t;

  server_base();
  virtual ~server_base() {}

  virtual mixer::mixer* get_mixer() const = 0;
  virtual status_t get_status() const = 0;

  virtual void save(const std::string& path, const std::string& id);
  virtual void load(const std::string& path, const std::string& id);
  void event_model_updated();

protected:
  virtual std::vector<pfi::lang::shared_ptr<mixable0> > get_mixables() const = 0;

  uint64_t update_count() const {
    return update_count_;
  }

private:
  uint64_t update_count_;
};

}
}
