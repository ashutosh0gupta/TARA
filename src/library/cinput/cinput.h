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

#include "helpers/helpers.h"
#include "helpers/z3interf.h"

namespace tara {
namespace cinput {
  class loc{
  public:
    unsigned line;
    unsigned col;
    std::string file;
  };

  class program {
  public:
    program(helpers::z3interf& z3_): z3(z3_) {}
    helpers::z3interf& z3;
    cssa::variable_set global;
    z3::expr phi_ssa = z3.c.bool_val(true);
    std::map< unsigned, loc> inst_to_loc;
    std::map< unsigned, std::string> id_to_thr_name;
    unsigned thread_num = 0;
    unsigned add_thread( std::string str) {
      id_to_thr_name[thread_num++] = str;
      return thread_num-1;
    };
  };

  program* parse_cpp_file( helpers::z3interf& z3_, std::string& cfile );

  // class cinput_exception : public std::runtime_error{
  // public:
  //   input_exception(const char* what) : runtime_error(what) {};
  //   input_exception(const std::string& what) : runtime_error(what) {};
  // };

}}
#endif // TARA_CINPUT_PROGRAM_H
