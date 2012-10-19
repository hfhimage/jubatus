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

#include <vector>
#include "../mixable.hpp"
#include "mixer.hpp"

namespace jubatus {
namespace framework {
namespace mixer {

class dummy_mixer : public mixer {
public:
  void register_api(pfi::network::mprpc::rpc_server& server) {}
  void register_mixable(mixable0* m) {
    mixables_.push_back(m);
  }

  void start() {}
  void stop() {}

  void updated() {}

  void get_status(server_base::status_t& status) const {}
  std::vector<mixable0*> get_mixables() const {
    return mixables_;
  }

private:
  std::vector<mixable0*> mixables_;
};

}
}
}
