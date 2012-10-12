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

#include <pficommon/network/mprpc.h>

namespace jubatus {
namespace framework {

class mixable0;

namespace mixer {

class mixer {
public:
  virtual ~mixer() {}

  virtual void register_api(pfi::network::mprpc::rpc_server& server) = 0;
  virtual void register_mixable(const pfi::lang::shared_ptr<mixable0>& m) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;

  virtual void updated() = 0;
};

}
}
}
