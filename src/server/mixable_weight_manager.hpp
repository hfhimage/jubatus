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

#include <pficommon/data/serialization.h>

#include "../fv_converter/weight_manager.hpp"
#include "../framework/mixable.hpp"
#include "../common/shared_ptr.hpp"

namespace jubatus{
namespace server{

using jubatus::fv_converter::weight_manager;
using jubatus::fv_converter::keyword_weights;

// TODO manage weight_manager pointer
struct mixable_weight_manager : public framework::mixable0
{
  mixable_weight_manager(jubatus::fv_converter::weight_manager* weight_manager = 0)
      : weight_manager_(weight_manager) {}

  // TODO use msgpack object or something instead of std::string
  std::string get_diff() const {
    if (weight_manager_) {
      std::ostringstream os;
      pfi::data::serialization::binary_oarchive bo(os);
      bo << const_cast<fv_converter::keyword_weights&>(weight_manager_->get_diff());
      return os.str();
    } else {
      return "";
    }
  }

  // TODO use msgpack object or something instead of std::string
  void put_diff(const std::string& diff) {
    if (weight_manager_) {
      keyword_weights w;
      std::istringstream is(diff);
      pfi::data::serialization::binary_iarchive bi(is);
      bi >> w;
      weight_manager_->put_diff(w);
    }
  }

  // TODO use msgpack object or something instead of std::string
  void reduce(const std::string& diff, std::string& acc) const {
    if (weight_manager_) {
      keyword_weights diff_weight, acc_weight;
      std::istringstream diff_is(diff), acc_is(acc);
      pfi::data::serialization::binary_iarchive diff_bi(diff_is), acc_bi(acc_is);
      diff_bi >> diff_weight;
      acc_bi >> acc_weight;
      acc_weight.merge(diff_weight);
      std::ostringstream os;
      pfi::data::serialization::binary_oarchive bo(os);
      bo << const_cast<fv_converter::keyword_weights&>(weight_manager_->get_diff());
      acc = os.str();
    }
  }

  void save(std::ostream& os) {
    if (weight_manager_) {
      pfi::data::serialization::binary_oarchive bo(os);
      bo << *weight_manager_;
    }
  }

  void load(std::istream& is) {
    if (weight_manager_) {
      pfi::data::serialization::binary_iarchive bi(is);
      bi >> *weight_manager_;
    }
  }

  void clear() {}

  fv_converter::weight_manager* weight_manager_;
};

}}
