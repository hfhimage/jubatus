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

#include <string>
#include <vector>

#include "../common/rpc_util.hpp"
#include "../classifier/classifier_base.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../storage/storage_base.hpp"
#include "../common/shared_ptr.hpp"

#include "classifier_types.hpp"
#include "../framework.hpp"
#include "diffv.hpp"
#include "mixable_weight_manager.hpp"

namespace jubatus{
namespace server{

// mixable object: It would be better if classifier object itself inherits framework::mixable<>
struct clsfer : public jubatus::framework::mixable<storage::storage_base, diffv>
{
  diffv get_diff_impl() const;

  void reduce_impl(const diffv& v, diffv& acc) const;

  void put_diff_impl(const diffv& v);

  void clear();

};

class classifier_serv : public framework::jubatus_serv
{
public:
  classifier_serv(const framework::server_argv&);  
  virtual ~classifier_serv();

  int set_config(config_data);
  config_data get_config();
  int train(std::vector<std::pair<std::string, datum> > data);
  std::vector<std::vector<estimate_result> > classify(std::vector<datum> data)const;

  void after_load();

  std::map<std::string, std::map<std::string, std::string> > get_status();

  void check_set_config()const;

private:
  config_data config_;
  pfi::lang::shared_ptr<fv_converter::datum_to_fv_converter> converter_;
  pfi::lang::shared_ptr<classifier_base> classifier_;
  clsfer clsfer_;
  mixable_weight_manager wm_;
};

}
} // namespace jubatus
