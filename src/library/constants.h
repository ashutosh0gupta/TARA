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
  regular,
  assume,
  assert,
};

extern const std::string error_label;

  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
  enum class mm_t {
    none, sc, tso, pso, rmo, power
      };
  std::string string_of_mm( mm_t mm );
  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------
}

#endif // TARA_CONSTANTS_H
