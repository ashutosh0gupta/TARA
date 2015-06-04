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


#include "program.h"
#include "helpers/helpers.h"
#include "input/input_exception.h"
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/algorithm/string.hpp>

using namespace tara;
using namespace tara::input;
using namespace tara::helpers;
using namespace std;

shared_ptr< instruction >& thread::operator[](unsigned int i)
{
  return instrs[i];
}

const shared_ptr< instruction >& thread::operator[](unsigned int i) const
{
  return instrs[i];
}


unsigned int thread::size() const
{
  return instrs.size();
}

thread::thread(string name) : name(name)
{ }


program::program()
{}


void program::convert_instructions(z3interf& z3) {
  printf("\nprogram:convert_instructions\n");
  for (unsigned i=0; i<threads.size(); i++)
    for (unsigned j=0; j<threads[i].size(); j++) {
      //instruction* in = instrs[i][j];
      if (shared_ptr<instruction_str> ins = dynamic_pointer_cast<instruction_str>(threads[i][j])) {
        variable_set havok_vars;
        for (string v : ins->havok_vars) {
          havok_vars.insert(find_variable(i, v));
        }
        // convert the string to a z3 expression
        shared_ptr<instruction_z3> newi = make_shared<instruction_z3>(ins->name, z3, ins->instr, set_union(threads[i].locals, globals), ins->type, havok_vars);
        threads[i][j] = newi;
      }
    }
    
    if (shared_ptr<instruction_str> ins = dynamic_pointer_cast<instruction_str>(precondition)) {
      precondition = make_shared<instruction_z3>(ins->name, z3, ins->instr, globals, ins->type, variable_set());
    }
    
    convert_names(z3);
}

void program::convert_names(z3interf& z3)
{
  printf("\nprogram:convert_names\n");
  if (names_converted) return;
  names_converted = true;
  
  vector<shared_ptr<hb_enc::location>> locations;
  
  // start location is needed to ensure all locations are mentioned in phi_po
  shared_ptr<hb_enc::location> start_location = make_shared<hb_enc::location>(z3.c, "start", true);
  _start_loc = start_location;
  locations.push_back(start_location);
  
  for (unsigned t=0; t<threads.size(); t++) {
    shared_ptr<hb_enc::location> prev;
    for (unsigned j=0; j<threads[t].size(); j++) {
      if (shared_ptr<instruction_z3> instr = dynamic_pointer_cast<instruction_z3>(threads[t][j])) {
        shared_ptr<hb_enc::location> loc = make_shared<hb_enc::location>(z3.c, instr->name);
        loc->thread = t;
        loc->instr_no = j;
        instr->_location = loc;
        
        // make sure these link to each other
        if (prev!=nullptr)
          prev->next = loc;
        loc->prev = prev;
        prev = loc;

        locations.push_back(loc);
      } else {
        throw input_exception("Instruction must be Z3");
      }
    }
  }
  z3.hb_encoding().make_locations(locations);
}


bool program::is_global(const variable& name) const
{
  return globals.find(variable(name))!=globals.end();
}

bool program::is_global(const string& name) const
{
  return is_global(variable(name, data_type::boolean));
}

variable program::find_variable(int thread, const string& name) const
{
  printf("\nprogram: find_variable\n");
  auto var = globals.find(variable(name, data_type::boolean));
  if (var!=globals.end()) {
    return *var;
  }
  if (thread >= 0 && thread < (int)threads.size()) {
    var = threads[thread].locals.find(variable(name, data_type::boolean));
    if (var!=threads[thread].locals.end()) {
      return *var;
    }
  }
  throw input_exception("Variable " + name + " is unknown");
}


void program::check_correctness()
{
  printf("\nprogram: check_correctness\n");
  unordered_set<string> thread_names; // check if the thread names are unique
  unordered_set<string> loc_names; // check if the location names are unique
  for (unsigned t=0; t<threads.size(); t++) {
    string& n = threads[t].name;
    if (thread_names.find(n) != thread_names.end()) 
      throw input_exception("Thread name \"" + n + "\" is duplicate");
    thread_names.insert(n);
    
    for (unsigned j=0; j<threads[t].size(); j++) {
      //instruction* in = instrs[i][j];
      if (shared_ptr<instruction_z3> ins = dynamic_pointer_cast<instruction_z3>(threads[t][j])) {
        // location name
        if (loc_names.find(ins->name) != loc_names.end()) 
          throw input_exception("Location name \"" + ins->name + "\" is duplicate");
        loc_names.insert(ins->name);
        
        unsigned primed = 0; // count primed variables
        // variables
        for(cssa::variable v : ins->variables()) {
          if (is_primed(v)) primed++;
          // check no primed variables in assume and assert
          if ((ins->type==instruction_type::assume || ins->type==instruction_type::assert) && is_primed(v)) {
            throw input_exception("Primed variables are not allowed in assumptions and assertions.");
          }
          // check if it is declared
          v = get_unprimed(v);
          if (!is_global(v.name) && threads[t].locals.find(variable(v, data_type::boolean))==threads[t].locals.end()) {
            throw input_exception("Variable " + v.name + " not declared");
          }
        }
        
        for (cssa::variable v : ins->havok_vars) {
          // check only unprimed variables in havok
          if (is_primed(v)) {
            throw input_exception("Unprimed variables are not allowed in havok.");
          }
          find_variable(t,v.name);
        }
        
      } else {
        throw input_exception("Instruction not in Z3 format found");
      }
    }
  }
  
  // check if all variables in the precondition are global
  if (shared_ptr<instruction_z3> ins = dynamic_pointer_cast<instruction_z3>(precondition)) {
    for (const cssa::variable& v: ins->variables()) {
      if (is_primed(v)) 
        throw input_exception("Primed variable found in precondition");
      if (!is_global(v)) 
        throw input_exception("Variable \"" + v.name + "\" used in precondition, but not global");
    }   
    if (ins->havok_vars.size() > 0)
      throw input_exception("Precondition may not havok.");
  } else {
    throw input_exception("Precondition is not in Z3 format");
  }
  
  // check if thread names are unique
  unordered_set<string> names;
  for (unsigned t=0; t<threads.size(); t++) {
    string& n = threads[t].name;
    if (names.find(n) != names.end()) 
      throw input_exception("Thread name \"" + n + "\" is duplicate");
    names.insert(n);
  }
  
  // check if atomic sections and happens-befores describe valid locations
  for (location_pair as: atomic_sections) {
    if (loc_names.find(get<0>(as))==loc_names.end() || loc_names.find(get<1>(as))==loc_names.end()) {
      throw input_exception("Atomic section [" + get<0>(as) + "," + get<1>(as) + "] has unknown locations.");
    }
  }
  for (location_pair as: happens_befores) {
    if (loc_names.find(get<0>(as))==loc_names.end() || loc_names.find(get<1>(as))==loc_names.end()) {
      throw input_exception("Order " + get<0>(as) + "<" + get<1>(as) + " has unknown locations.");
    }
  }

}


