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

//----------------------------------------------------------------------------
// start of wmm support
//----------------------------------------------------------------------------

bool program::is_wmm() const
{
  return mm != mm_t::sc || mm != mm_t::none;
}

bool program::is_mm_sc() const
{
  return mm == mm_t::sc;
}

bool program::is_mm_tso() const
{
  return mm == mm_t::sc;
}

bool program::is_mm_pso() const
{
  return mm == mm_t::pso;
}

bool program::is_mm_rmo() const
{
  return mm == mm_t::rmo;
}

bool program::is_mm_alpha() const
{
  return mm == mm_t::alpha;
}

bool program::is_mm_power() const
{
  return mm == mm_t::power;
}

void program::set_mm(mm_t _mm)
{
  mm = _mm;
}

mm_t program::get_mm() const
{
  return mm;
}

//----------------------------------------------------------------------------
// end of wmm support
//----------------------------------------------------------------------------

void program::convert_instructions(z3interf& z3, hb_enc::encoding& hb_enc) {
  auto all_variables = globals;
  for (unsigned i=0; i<threads.size(); i++) {
    for( auto& local: threads[i].locals ) {
      variable v1( threads[i].name + "." + local.name, local.type );
      all_variables.insert(v1);
    }
    for (unsigned j=0; j<threads[i].size(); j++) {
      //instruction* in = instrs[i][j];
      if (shared_ptr<instruction_str> ins = dynamic_pointer_cast<instruction_str>(threads[i][j])) {
        input::variable_set havok_vars;
        for (string v : ins->havok_vars) {
          havok_vars.insert(find_variable(i, v));
        }
        // convert the string to a z3 expression
        shared_ptr<instruction_z3> newi = make_shared<instruction_z3>(ins->name, z3, ins->instr, set_union(threads[i].locals, globals), ins->type, havok_vars);
        threads[i][j] = newi;
      }
    }
  }
    
    if (shared_ptr<instruction_str> ins = dynamic_pointer_cast<instruction_str>(precondition)) {
      precondition = make_shared<instruction_z3>(ins->name, z3, ins->instr, globals, ins->type, variable_set());
    }

    if (shared_ptr<instruction_str> post = dynamic_pointer_cast<instruction_str>(postcondition)) {
      postcondition = make_shared<instruction_z3>(post->name, z3, post->instr, all_variables, post->type, variable_set());
    }
    
    convert_names(z3, hb_enc );
}

void program::convert_names(z3interf& z3, hb_enc::encoding& hb_enc)
{
  if (names_converted) return;
  names_converted = true;
  
  vector<hb_enc::tstamp_var_ptr> locations;
  
  // start tstamp is needed to ensure all tstamps are mentioned in phi_po
  hb_enc::tstamp_var_ptr start_location = make_shared<hb_enc::tstamp>(z3.c, "start", true);
  _start_loc = start_location;
  locations.push_back(start_location);

  hb_enc::tstamp_var_ptr end_location = make_shared<hb_enc::tstamp>(z3.c, "end", true);
  _end_loc = end_location;
  locations.push_back(end_location);
  
  for (unsigned t=0; t<threads.size(); t++) {
    hb_enc::tstamp_var_ptr prev;
    for (unsigned j=0; j<threads[t].size(); j++) {
      if (shared_ptr<instruction_z3> instr = dynamic_pointer_cast<instruction_z3>(threads[t][j])) {
        hb_enc::tstamp_var_ptr loc = make_shared<hb_enc::tstamp>(z3.c, instr->name);
        loc->thread = t;
        loc->instr_no = j;
        instr->_tstamp = loc;
        
        // make sure these link to each other
        if (prev!=nullptr)
          prev->next = loc;
        loc->prev = prev;
        prev = loc;

        locations.push_back(loc);
      } else {
        ctrc_input_error("Instruction must be Z3");
      }
    }
  }
  //z3.hb_encoding().
  hb_enc.add_time_stamps(locations);
}


bool program::is_global(const input::variable& name) const
{
  return globals.find(input::variable(name))!=globals.end();
}

bool program::is_global(const string& name) const
{
  return is_global(input::variable(name, data_type::boolean));
}

input::variable program::find_variable(int thread, const string& name) const
{
  auto var = globals.find(input::variable(name, data_type::boolean));
  if (var!=globals.end()) {
    return *var;
  }
  if (thread >= 0 && thread < (int)threads.size()) {
    var = threads[thread].locals.find(input::variable(name, data_type::boolean));
    if (var!=threads[thread].locals.end()) {
      return *var;
    }
  }
  ctrc_input_error("Variable " + name + " is unknown");
}


void program::check_correctness()
{
  unordered_set<string> thread_names; // check if the thread names are unique
  unordered_set<string> loc_names; // check if the location names are unique
  for (unsigned t=0; t<threads.size(); t++) {
    string& n = threads[t].name;
    if (thread_names.find(n) != thread_names.end()) 
      ctrc_input_error("Thread name \"" + n + "\" is duplicate");
    thread_names.insert(n);
    
    for (unsigned j=0; j<threads[t].size(); j++) {
      //instruction* in = instrs[i][j];
      if (shared_ptr<instruction_z3> ins = dynamic_pointer_cast<instruction_z3>(threads[t][j])) {
        // location name
        if (loc_names.find(ins->name) != loc_names.end()) 
          ctrc_input_error("Location name \"" + ins->name + "\" is duplicate");
        loc_names.insert(ins->name);
        
        unsigned primed = 0; // count primed variables
        // variables
        for(auto v : ins->variables()) {
          if (is_primed(v)) primed++;
          // check no primed variables in assume and assert
          if ((ins->type==instruction_type::assume || ins->type==instruction_type::assert) && is_primed(v)) {
            ctrc_input_error("Primed variables are not allowed in assumptions and assertions.");
          }
          // check if it is declared
          v = get_unprimed(v);
          if (!is_global(v.name) && threads[t].locals.find(variable(v, data_type::boolean))==threads[t].locals.end()) {
            ctrc_input_error("Variable " + v.name + " not declared");
          }
        }
        
        for (auto v : ins->havok_vars) {
          // check only unprimed variables in havok
          if (is_primed(v)) {
            ctrc_input_error("Unprimed variables are not allowed in havok.");
          }
          find_variable(t,v.name);
        }
        
      } else {
        ctrc_input_error("Instruction not in Z3 format found");
      }
    }
  }
  
  // check if all variables in the precondition are global
  if (shared_ptr<instruction_z3> ins = dynamic_pointer_cast<instruction_z3>(precondition)) {
    for (const auto& v: ins->variables()) {
      if (is_primed(v)) 
        ctrc_input_error("Primed variable found in precondition");
      if (!is_global(v)) 
        ctrc_input_error("Variable \"" + v.name + "\" used in precondition, but not global");
    }   
    if (ins->havok_vars.size() > 0)
      ctrc_input_error("Precondition may not havok.");
  } else {
    ctrc_input_error("Precondition is not in Z3 format");
  }
  
  // check if thread names are unique
  unordered_set<string> names;
  for (unsigned t=0; t<threads.size(); t++) {
    string& n = threads[t].name;
    if (names.find(n) != names.end()) 
      ctrc_input_error("Thread name \"" + n + "\" is duplicate");
    names.insert(n);
  }
  
  // check if atomic sections and happens-befores describe valid locations
  for (location_pair as: atomic_sections) {
    if (loc_names.find(get<0>(as))==loc_names.end() || loc_names.find(get<1>(as))==loc_names.end()) {
      ctrc_input_error("Atomic section [" + get<0>(as) + "," + get<1>(as) + "] has unknown locations.");
    }
  }
  for (location_pair as: happens_befores) {
    if (loc_names.find(get<0>(as))==loc_names.end() || loc_names.find(get<1>(as))==loc_names.end()) {
      ctrc_input_error("Order " + get<0>(as) + "<" + get<1>(as) + " has unknown locations.");
    }
  }

}


