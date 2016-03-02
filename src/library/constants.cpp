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

using namespace std;

namespace tara {
const string error_label = string("ERROR");

  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
  string string_of_mm( mm_t mm ) {
    switch( mm ){
    case mm_t::none:  return "unspecified";
    case mm_t::sc:    return "sc";
    case mm_t::tso:   return "tso";
    case mm_t::pso:   return "pso";
    case mm_t::rmo:   return "rmo";
    case mm_t::alpha: return "alpha";
    case mm_t::power: return "power";
    default: return "(so unsupproted that we forgot to give it a name!!)";
    }
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
  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------

}
