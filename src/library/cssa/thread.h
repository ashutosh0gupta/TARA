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

#ifndef TARA_CSSA_THREAD_H
#define TARA_CSSA_THREAD_H

#include <vector>
#include "helpers/z3interf.h"
#include "hb_enc/encoding.h"
#include <unordered_map>
#include "constants.h"
#include "cssa/wmm.h"

namespace tara {
namespace cssa {

class thread;

struct instruction {
  hb_enc::location_ptr loc;
  z3::expr instr;
  z3::expr path_constraint;
  thread* in_thread;
  std::string name;
  variable_set variables_read_copy;
  variable_set variables_read; // the variable names with the hash
  variable_set variables_write; // the variable names with the hash
  variable_set variables_read_orig; // the original variable names
  variable_set variables_write_orig; // the original variable names, unprimed
  variable_set variables() const;
  variable_set variables_orig() const;
  instruction_type type;
  variable_set havok_vars;

  //--------------------------------------------------------------------------
  // WMM support
  //--------------------------------------------------------------------------

  se_set rds;
  se_set wrs;
  se_set barr; //the event created for fence if the instruction is fence type
public:
  //--------------------------------------------------------------------------
  // End WMM support
  //--------------------------------------------------------------------------
  
  instruction(helpers::z3interf& z3, hb_enc::location_ptr location, thread* thread, std::string& name, instruction_type type, z3::expr original_expression);
  friend std::ostream& operator<< (std::ostream& stream, const instruction& i);
  void debug_print( std::ostream& o );
private:
  z3::expr original_expr;
};
  
class thread
{
public:
  thread(const std::string& name, variable_set locals);
  thread(thread& ) = delete;
  thread& operator=(thread&) = delete;
private:
public:

  std::vector<std::shared_ptr<instruction>> instructions;
  const std::string name;
  std::unordered_map<std::string,std::vector<std::shared_ptr<instruction>>> global_var_assign;
  variable_set locals;
  
  bool operator==(const thread &other) const;
  bool operator!=(const thread &other) const;
  
  unsigned size() const;
  instruction& operator [](unsigned i);
  const instruction& operator [](unsigned i) const;
  void add_instruction(const std::shared_ptr< tara::cssa::instruction >& instr);
};
}}

#endif // TARA_CSSA_THREAD_H
