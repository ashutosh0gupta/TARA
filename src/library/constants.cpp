/*
 * Copyright 2014, IST Austria
 *
 * This file is part of TARA.
 *
 * TARA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TARA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with TARA.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "constants.h"
#include <set>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <cassert>

using namespace std;

namespace tara {
const string error_label = string("ERROR");

  std::set< std::pair<mm_t, std::string> > mm_names = {
    {mm_t::none , "none"  },
    {mm_t::sc   , "sc"    },
    {mm_t::tso  , "tso"   },
    {mm_t::pso  , "pso"   },
    {mm_t::rmo  , "rmo"   },
    {mm_t::alpha, "alpha" },
    {mm_t::power, "power" },
    {mm_t::c11  , "c11"   },
    {mm_t::arm8_2, "arm8.2"} // needs renaming
  };

  string string_of_mm( mm_t mm ) {
    for( auto& pr : mm_names ) {
      if( pr.first == mm ) {
        return pr.second;
      }
    }
    return "(so unsupproted memory model that we forgot to give it a name!!)";
  }

  string string_of_mm_names() {
    bool first = true;
    std::stringstream ss;
    for( auto pr : mm_names) {
      if(first) first = false; else ss << ",";
      ss << "\"" << pr.second << "\"";
    }
    return ss.str();
  }

  mm_t mm_of_string( string s ) {
    for( auto pr : mm_names ) {
      if( pr.second == s ) {
        return pr.first;
      }
    }
    return mm_t::wrong;
  }


  bool is_barrier(instruction_type type) {
    return type == instruction_type::fence ||
      type == instruction_type::sync ||
      type == instruction_type::lwsync ||
      type == instruction_type::dmb ||
      type == instruction_type::barrier ||
      type == instruction_type::barrier_a ||
      type == instruction_type::barrier_b;
  }

  bool mm_needs_data_dependency( mm_t mm ) {
    assert( mm == mm_t::none
            || mm == mm_t::sc
            || mm == mm_t::tso
            || mm == mm_t::pso
            || mm == mm_t::rmo
            || mm == mm_t::alpha
            || mm == mm_t::power
            || mm == mm_t::c11
            || mm == mm_t::arm8_2
            );
    //todo: Do tso/pso need thin-air value constraints?
    // if no they also do not need data dependency
    return mm != mm_t::sc;
  }

  bool mm_needs_ctrl_dependency( mm_t mm ) {
    assert( mm == mm_t::none
            || mm == mm_t::sc
            || mm == mm_t::tso
            || mm == mm_t::pso
            || mm == mm_t::rmo
            || mm == mm_t::alpha
            || mm == mm_t::power
            || mm == mm_t::c11
            || mm == mm_t::arm8_2
            );
    return mm_t::rmo == mm || mm_t::arm8_2 == mm || mm_t::power == mm;
  }

  bool mm_needs_addr_dependency( mm_t mm ) {
    assert(false); // noone ever called this function
    assert( mm == mm_t::none
            || mm == mm_t::sc
            || mm == mm_t::tso
            || mm == mm_t::pso
            || mm == mm_t::rmo
            || mm == mm_t::alpha
            || mm == mm_t::power
            || mm == mm_t::c11
            || mm == mm_t::arm8_2
            );
    return mm_t::rmo == mm || mm_t::arm8_2 == mm || mm_t::power == mm;
  }


}
