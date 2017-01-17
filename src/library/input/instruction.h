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

#ifndef TARA_INPUT_INSTRUCTION_H
#define TARA_INPUT_INSTRUCTION_H

#include <unordered_set>
#include <string>
#include <unordered_set>
#include <z3++.h>
#include "helpers/z3interf.h"
#include "constants.h"
#include "hb_enc/integer.h"

namespace tara {
namespace input {

  
class instruction
{
public:
  std::string name;
  instruction(const std::string& name);
  virtual ~instruction() {};
};


class instruction_str: public instruction {
public:
  instruction_str(const std::string& name, const std::string& instr, instruction_type type, string_set havok_vars);
  std::string instr;
  instruction_type type;
  string_set havok_vars;
};

class instruction_z3 : public instruction {
public:
  instruction_z3(const std::string& name, const z3::expr& instr, instruction_type type, cssa::variable_set variables, cssa::variable_set havok_vars);
  instruction_z3(const std::string& name, const z3::expr& instr, instruction_type type, cssa::variable_set havok_vars);
  instruction_z3(const std::string& name, tara::helpers::z3interf& z3, const std::string& instr, variable_set variables, tara::instruction_type type, variable_set havok_vars);
  z3::expr instr;
  inline hb_enc::tstamp_ptr location() {return _tstamp;}
  
  /**
   * @brief variables read/changed in this instruction
   * 
   * @return tara::cssa::variable_set
   */
  cssa::variable_set variables();
  /**
   * @brief Is this an assertion or assumption or regular statement
   * 
   */
  instruction_type type;
  /**
   * @brief The set of variables that will be havoked in this statement (they are treated as inputs)
   * 
   */
  cssa::variable_set havok_vars;
  hb_enc::tstamp_var_ptr _tstamp;
private:
  cssa::variable_set _variables;

  friend class program;
};

}}

#endif // TARA_INPUT_INSTRUCTION_H
