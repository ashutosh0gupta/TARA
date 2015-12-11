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
#include "cssa_exception.h"
#include "helpers/helpers.h"
#include "helpers/int_to_string.h"
#include <string.h>
#include "helpers/z3interf.h"
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

pi_function_part::pi_function_part(variable_set variables, z3::expr hb_exression) : variables(variables), hb_exression(hb_exression)
{}

pi_function_part::pi_function_part(z3::expr hb_exression) : hb_exression(hb_exression)
{}

void program::build_threads(const input::program& input)
{
  z3::context& c = _z3.c;
  vector<pi_needed> pis;

  unordered_map<string, int> count_occur; // for counting occurences of each variable

  for (string v: input.globals) {
      count_occur[v]=0;
    }


    unordered_map<string, string> thread_vars; // maps variable to the last place they were assigned

  for (unsigned t=0; t<input.threads.size(); t++) {
    unordered_map<string, shared_ptr<instruction>> global_in_thread; // last reference to global variable in this thread (if any)


    // set all local vars to "pre"
    for (string v: input.threads[t].locals) {
      thread_vars[v] = "pre";
      count_occur[v]=0;
    }

    z3::expr path_constraint = c.bool_val(true);
    variable_set path_constraint_variables;
    thread& thread = *threads[t];

    for (unsigned i=0; i<input.threads[t].size(); i++)
    {
      if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.threads[t][i]))
      {
        z3::expr_vector src(c);
        z3::expr_vector dst(c);
        for(const variable& v: instr->variables())
        {
          variable nname(c);

          if (!is_primed(v))
          {

            if (is_global(v))
            {
              count_occur[v]++;
              nname =v + int_to_string(count_occur[v]);
              std::shared_ptr<tara::cssa::variable> var_ptr=make_shared<tara::cssa::variable>(nname);
              pis.push_back(pi_needed(nname, v, global_in_thread[v], t, thread[i].loc));
            }
            else
            {
              nname = v + int_to_string(count_occur[v]) ;
              std::shared_ptr<tara::cssa::variable> var_ptr=make_shared<tara::cssa::variable>(nname);
              // check if we are reading an initial value
              if (thread_vars[v] == "pre")
              {
                initial_variables.insert(nname);

              }
            }

            thread[i].variables_read.insert(nname); // this variable is read by this instruction
            thread[i].variables_read_orig.insert(v);
          }
          else
          {

            variable v1 = get_unprimed(v); //converting the primed to unprimed by removing the dot
            count_occur[v1]++;
            if(thread_vars[v1]=="pre")
            {    // every lvalued variable gets incremented by 1 and then the local variables are decremented by 1
              count_occur[v1]--;
            }
            if (is_global(v1))
           {
              //nname = v1 + "#" + thread[i].loc->name;
              nname = v1 + int_to_string(count_occur[v1]);
              std::shared_ptr<tara::cssa::variable> var_ptr=make_shared<tara::cssa::variable>(nname);

            }
            else
            {
              //nname = thread.name + "." + v1 + "#" + thread[i].loc->name;
              nname = v1 + int_to_string(count_occur[v1]);
              std::shared_ptr<tara::cssa::variable> var_ptr=make_shared<tara::cssa::variable>(nname);
              //instr_to_var[threads[t]->instructions[i]]=var_ptr;
            }
            thread[i].variables_write.insert(nname); // this variable is written by this instruction
            thread[i].variables_write_orig.insert(v1);
            // map the instruction to the variable
            variable_written[nname] = thread.instructions[i];
          }
          src.push_back(v);
          dst.push_back(nname);
        }

        // tread havoked variables the same
        for(const variable& v: instr->havok_vars) {
          variable nname(c);
          variable v1 = get_unprimed(v);
          thread[i].havok_vars.insert(v1);
          if (is_global(v1)) {
            nname = v1 + "#" + thread[i].loc->name;
          } else {
            nname = thread.name + "." + v1 + "#" + thread[i].loc->name;
          }
          thread[i].variables_write.insert(nname); // this variable is written by this instruction
          thread[i].variables_write_orig.insert(v1);
          // map the instruction to the variable
          variable_written[nname] = thread.instructions[i];
          src.push_back(v);
          dst.push_back(nname);
          // add havok to initial variables
          initial_variables.insert(nname);
        }
        // replace variables in expr
        z3::expr newi = instr->instr.substitute(src,dst);
        // deal with the path-constraint
        thread[i].instr = newi;
        if (thread[i].type == instruction_type::assume) {
          path_constraint = path_constraint && newi;
          path_constraint_variables.insert(thread[i].variables_read.begin(), thread[i].variables_read.end());
        }
        thread[i].path_constraint = path_constraint;
        thread[i].variables_read.insert(path_constraint_variables.begin(), path_constraint_variables.end());
        if (instr->type == instruction_type::regular) {
          phi_vd = phi_vd && newi;
        }
        else if (instr->type == instruction_type::assert) {
          // add this assertion, but protect it with the path-constraint

          phi_prp = phi_prp && implies(path_constraint,newi);
          // add used variables
          assertion_instructions.insert(thread.instructions[i]);
        }
        // increase referenced variables
        //for(variable v: thread[i].variables_write_orig) {
        for(variable v: thread[i].variables_write)
        {
          thread_vars[v] = thread[i].loc->name;
          if (is_global(v)) {
            global_in_thread[v] = thread.instructions[i];
            thread.global_var_assign[v].push_back(thread.instructions[i]);
          }
        }
        // for(variable v:thread[i].variables_read)
        // {
        //   std::cout<<"\nnname.name["<<i<<"]: "<<v.name<<"\n";
        // }

        for(auto itr=pis.begin();itr!=pis.end();itr++)
        {
          if(itr->last_local!=NULL)
          {
              //std::cout<<" pis itr"<<(itr)->last_local->instr<<"\n";
          }
        }
      }
      else {
        throw cssa_exception("Instruction must be Z3");
      }
    }
    phi_fea = phi_fea && path_constraint;
  }

  //variables_read_copy(variables_read);
  for(unsigned int k=0;k<threads.size();k++)
      {
        for(unsigned int n=0;n<input.threads[k].size();n++)
        {
          for(variable vn:(threads[k]->instructions[n])->variables_read_orig)
          {
            for(variable vo:(threads[k]->instructions[n])->variables_read)
            {
              if(check_correct_global_variable(vo,vn.name))
              {
                  (threads[k]->instructions[n])->variables_read_copy.insert(vo);
              }
            }

            //if((threads[k]->instructions[n])->name)
            //

          }

        }
      }




   shared_ptr<input::instruction_z3> pre_instr = dynamic_pointer_cast<input::instruction_z3>(input.precondition);

  build_ses(input,c);
  build_pis(pis, input);
}


void program::build_pis(vector< program::pi_needed >& pis, const input::program& input) {
  z3::context& c = _z3.c;
  for (pi_needed pi : pis) {
    variable nname = pi.name;
    vector<pi_function_part> pi_parts;
    // get a list of all locations in question
    vector<shared_ptr<const instruction>> locs;
    if (pi.last_local != nullptr) {
      locs.push_back(pi.last_local);
    }
    for (unsigned t = 0; t<threads.size(); t++) {
      if (t!=pi.thread) {
        locs.insert(locs.end(), threads[t]->global_var_assign[pi.orig_name].begin(),threads[t]->global_var_assign[pi.orig_name].end());
        }
    }

    z3::expr p = c.bool_val(false);
    z3::expr p_hb = c.bool_val(true);
    // reading from pre part
    // only if the variable was never assigned in this thread we can read from pre
    if (pi.last_local == nullptr) {
      //p = p || (z3::expr)nname == (pi.orig_name + "#pre");
      p = p || (z3::expr)nname == (pi.orig_name + "0");
      for(const shared_ptr<const instruction>& lj: locs) {
        assert (pi.loc->thread!=lj->loc->thread);
        p_hb = p_hb && (_hb_encoding.make_hb(pi.loc,lj->loc));
      }
      pi_parts.push_back(pi_function_part(p_hb));
      p = p && p_hb;
      // add to initial variables list

      //initial_variables.insert(pi.orig_name + "#pre");

      initial_variables.insert(pi.orig_name + "0");
    }

    // for all other locations
    for (const shared_ptr<const instruction>& li : locs) {
      p_hb = c.bool_val(true);
      variable oname = pi.orig_name + "#" + li->loc->name;
      //variable oname = pi.name;
      variable_set vars = li->variables_read; // variables used by this part of the pi function, init with variables of the path constraint
      vars.insert(oname);
      z3::expr p1 = (z3::expr)nname == oname;
      p1 = p1 && li->path_constraint;
      if (!pi.last_local || li->loc != pi.last_local->loc) { // ignore if this is the last location this was written
        assert (pi.loc->thread!=li->loc->thread);
        p_hb = p_hb && _hb_encoding.make_hb(li->loc, pi.loc);
      }
      for(const shared_ptr<const instruction>& lj: locs) {
        if (li->loc != lj->loc) {
          z3::expr before_write(_z3.c);
          z3::expr after_read(_z3.c);
          if (*(li->in_thread) == *(lj->in_thread)) {
            // we can immediatelly determine if this is true
            assert(li->loc != lj->loc);
            before_write = _z3.c.bool_val(lj->loc->instr_no < li->loc->instr_no);
          } else {
            assert (li->loc->thread!=lj->loc->thread);
            before_write = _hb_encoding.make_hb(lj->loc,li->loc);
          }
          if (!pi.last_local || lj->loc != pi.last_local->loc) { // ignore if this is the last location this was written
            assert (pi.loc->thread!=lj->loc->thread);
            after_read = _hb_encoding.make_hb(pi.loc,lj->loc);
          } else
            after_read = _z3.c.bool_val(pi.loc->instr_no < lj->loc->instr_no);
          p_hb = p_hb && (before_write || after_read);
        }
      }
      p = p || (p1 && p_hb);
      pi_parts.push_back(pi_function_part(vars,p_hb));
    }
    phi_pi = phi_pi && p;
    pi_functions[nname] = pi_parts;
  }
}


void program::build_hb(const input::program& input)
{
  z3::expr_vector locations(_z3.c);
  // start location is needed to ensure all locations are mentioned in phi_po
  shared_ptr<hb_enc::location> start_location = input.start_name();
  locations.push_back(*start_location);

  for (unsigned t=0; t<input.threads.size(); t++) {
    thread& thread = *threads[t];
    shared_ptr<hb_enc::location> prev;
    for (unsigned j=0; j<input.threads[t].size(); j++) {
      if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j])) {
        shared_ptr<hb_enc::location> loc = instr->location();
        locations.push_back(*loc);

        std::shared_ptr<instruction> ninstr = make_shared<instruction>(_z3, loc, &thread, instr->name, instr->type, instr->instr);
        thread.add_instruction(ninstr);

      } else {
        throw cssa_exception("Instruction must be Z3");
      }
    }
  }

  // make a distinct attribute
  phi_distinct = distinct(locations);

  for(unsigned t=0; t<threads.size(); t++) {
    thread& thread = *threads[t];
    unsigned j=0;
    phi_po = phi_po && _hb_encoding.make_hb(start_location, thread[0].loc);
    for(j=0; j<thread.size()-1; j++) {
      phi_po = phi_po && _hb_encoding.make_hb(thread[j].loc, thread[j+1].loc);
    }
  }

  // add atomic sections
  for (input::location_pair as : input.atomic_sections) {
    hb_enc::location_ptr loc1 = _hb_encoding.get_location(get<0>(as));
    hb_enc::location_ptr loc2 = _hb_encoding.get_location(get<1>(as));
    phi_po = phi_po && _hb_encoding.make_as(loc1, loc2);
  }
  // add happens-befores
  for (input::location_pair hb : input.happens_befores) {
    hb_enc::location_ptr loc1 = _hb_encoding.get_location(get<0>(hb));
    hb_enc::location_ptr loc2 = _hb_encoding.get_location(get<1>(hb));
    phi_po = phi_po && _hb_encoding.make_hb(loc1, loc2);
  }
  phi_po = phi_po && phi_distinct;
}

void program::build_pre(const input::program& input)
{

  if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.precondition)) {
    z3::expr_vector src(_z3.c);
    z3::expr_vector dst(_z3.c);
    for(const variable& v: instr->variables()) {
      variable nname(_z3.c);
      if (!is_primed(v)) {
        //nname = v + "#pre";
        nname = v + "0";
        new_globals.insert(nname);
      } else {
        throw cssa_exception("Primed variable in precondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_pre = instr->instr.substitute(src,dst);
  } else {
    throw cssa_exception("Instruction must be Z3");
  }
}


program::program(z3interf& z3, hb_enc::encoding& hb_encoding, const input::program& input):
  _z3(z3), _hb_encoding(hb_encoding), globals(z3.translate_variables(input.globals))
{
  // add threads
  for (unsigned t=0; t<input.threads.size(); t++) {
    shared_ptr<thread> tp = make_shared<thread>(input.threads[t].name, z3.translate_variables(input.threads[t].locals));
    threads.push_back(move(tp));
  }

  build_hb(input);
  build_pre(input);
  build_threads(input);
}

const thread& program::operator[](unsigned int i) const
{
  return *threads[i];
}

unsigned int program::size() const
{
  return threads.size();
}

bool program::is_global(const variable& name) const
{
  return globals.find(variable(name))!=globals.end();
}

const instruction& program::lookup_location(const hb_enc::location_ptr& location) const
{
  return (*this)[location->thread][location->instr_no];
}


std::vector< std::shared_ptr<const instruction> > program::get_assignments_to_variable(const variable& variable) const
{
  string name = (get_unprimed(variable)).name;
  vector<std::shared_ptr<const instruction>> result;
  for (unsigned i = 0; i<this->size(); i++) {
    const cssa::thread& t = (*this)[i];

    auto find = t.global_var_assign.find(name);
    if (find != t.global_var_assign.end()) {
      const auto& instrs = get<1>(*find);
      result.insert(result.end(), instrs.begin(), instrs.end());
    }
  }
  return result;
}
