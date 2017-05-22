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

#ifndef TARA_CONSTANTS_H
#define TARA_CONSTANTS_H

#include <string>

namespace tara {
enum class instruction_type {
  regular,//todo: extend it for rmw instructions
  assume,
  assert,
  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
    fence,sync,lwsync,dmb,barrier,barrier_a,barrier_b
  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------
};

extern const std::string error_label;

  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
  enum class mm_t {
    wrong, none, sc, tso, pso, rmo, alpha, power, c11, arm8_2
  };
  std::string string_of_mm_names();
  std::string string_of_mm( mm_t mm );
  mm_t mm_of_string( std::string s );

  bool is_barrier(instruction_type type);
  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------
}

#endif // TARA_CONSTANTS_H
