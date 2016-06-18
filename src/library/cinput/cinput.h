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
 */

#ifndef TARA_CINPUT_PROGRAM_H
#define TARA_CINPUT_PROGRAM_H

#include "helpers/z3interf.h"

namespace tara {
namespace cinput {
  class program {
  public:
    program(helpers::z3interf& z3_): z3(z3_) {}
    helpers::z3interf& z3;
    z3::expr phi_ssa = z3.c.bool_val(true);
  };

  program* parse_cpp_file( helpers::z3interf& z3_, std::string& cfile );

}}
#endif // TARA_CINPUT_PROGRAM_H
