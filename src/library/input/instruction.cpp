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


#include "instruction.h"

#include "input_exception.h"

using namespace tara::input;
using namespace tara;

using namespace std;

instruction::instruction(const string& name) : name(name)
{ }

instruction_str::instruction_str(const string& name, const string& instr, instruction_type type, string_set havok_vars) : instruction(name), instr(instr), type(type), havok_vars(havok_vars)
{}

instruction_z3::instruction_z3(const string& name, helpers::z3interf& z3, const string& instr, variable_set variables, instruction_type type, variable_set havok_vars) :
instruction_z3(name, z3.parseFormula(instr,variables), type, z3.translate_variables(havok_vars))
{}

instruction_z3::instruction_z3(const string& name, const z3::expr& instr, instruction_type type, cssa::variable_set havok_vars):
instruction_z3(name, instr, type, helpers::z3interf::get_variables(instr), havok_vars)
{}

instruction_z3::instruction_z3(const string& name, const z3::expr& instr, instruction_type type, cssa::variable_set variables, cssa::variable_set havok_vars): 
instruction(name), instr(instr), type(type), havok_vars(havok_vars), _variables(variables)
{}



cssa::variable_set instruction_z3::variables()
{
  return _variables;
}



