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


#include "program/program.h"
#include "program.h"
#include "cssa_exception.h"
#include "helpers/helpers.h"
#include <vector>
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

pi_function_part::pi_function_part(variable_set variables, z3::expr hb_exression) : variables(variables), hb_exression(hb_exression)
{}

pi_function_part::pi_function_part(z3::expr hb_exression) : hb_exression(hb_exression)
{}

void cssa::program::build_threads(const input::program& input)
{
  z3::context& c = _z3.c;
  vector<pi_needed> pis;
  for (unsigned t=0; t<input.threads.size(); t++) {
    unordered_map<string, shared_ptr<instruction>> global_in_thread; // last reference to global variable in this thread (if any)
    unordered_map<string, string> thread_vars; // maps variable to the last place they were assigned
    // set all local vars to "pre"
    for (string v: input.threads[t].locals) {
      thread_vars[v] = "pre";
    }
    
    z3::expr path_constraint = c.bool_val(true);
    variable_set path_constraint_variables;
    tara::thread& thread = *threads[t];
    for (unsigned i=0; i<input.threads[t].size(); i++) {
      if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.threads[t][i])) {
        z3::expr_vector src(c);
        z3::expr_vector dst(c);
        for(const variable& v: instr->variables()) {
          variable nname(c);
          if (!is_primed(v)) {
            if (is_global(v)) {
              nname = "pi_" + v + "#" + thread[i].loc->name;
              pis.push_back(pi_needed(nname, v, global_in_thread[v], t, thread[i].loc));
            } else {
              nname = thread.name + "." + v + "#" + thread_vars[v];
              // check if we are reading an initial value
              if (thread_vars[v] == "pre") {
                initial_variables.insert(nname);
              }
            }
            thread[i].variables_read.insert(nname); // this variable is read by this instruction
            thread[i].variables_read_orig.insert(v);
          } else {
            variable v1 = get_unprimed(v);
            if (is_global(v1)) {
              nname = v1 + "#" + thread[i].loc->name;
            } else {
              nname = thread.name + "." + v1 + "#" + thread[i].loc->name;
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
//        if(thread[i].type == instruction_type::fence)
//        {
//        	instr_to_barr.insert({i,instr->type});
//        	tid_to_barr.insert({t,instr->type});
//
//        }
        thread[i].path_constraint = path_constraint;
        thread[i].variables_read.insert(path_constraint_variables.begin(), path_constraint_variables.end());
        if (instr->type == instruction_type::regular) {
          phi_vd = phi_vd && newi;
        } else if (instr->type == instruction_type::assert) {
          // add this assertion, but protect it with the path-constraint
          phi_prp = phi_prp && implies(path_constraint,newi);
          // add used variables
          assertion_instructions.insert(thread.instructions[i]);
        }
        // increase referenced variables
        for(variable v: thread[i].variables_write_orig) {
          thread_vars[v] = thread[i].loc->name;
          if (is_global(v)) {
            global_in_thread[v] = thread.instructions[i];
            thread.global_var_assign[v].push_back(thread.instructions[i]);
          }
        }
      }
      else {
        throw cssa_exception("Instruction must be Z3");
      }
    }
    phi_fea = phi_fea && path_constraint;
  }
  build_pis(pis, input);
}

unsigned cssa::program::no_of_threads() const
{
	return threads.size();
}

const tara::thread& cssa::program::get_thread( unsigned tid ) const {
  return *threads[tid];
}


void cssa::program::build_pis(vector< cssa::program::pi_needed >& pis, const input::program& input)
{
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
      p = p || (z3::expr)nname == (pi.orig_name + "#pre");
      for(const shared_ptr<const instruction>& lj: locs) {
        assert (pi.loc->thread!=lj->loc->thread);
        p_hb = p_hb && (_hb_encoding.make_hb(pi.loc,lj->loc));
      }
      pi_parts.push_back(pi_function_part(p_hb));
      p = p && p_hb;
      // add to initial variables list
      initial_variables.insert(pi.orig_name + "#pre");
    }
    
    // for all other locations
    for (const shared_ptr<const instruction>& li : locs) {
      p_hb = c.bool_val(true);
      variable oname = pi.orig_name + "#" + li->loc->name;
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


void cssa::program::build_hb(const input::program& input)
{
  z3::expr_vector locations(_z3.c);
  // start location is needed to ensure all locations are mentioned in phi_po
  shared_ptr<hb_enc::location> start_location = input.start_name();
  locations.push_back(*start_location);
  
  for (unsigned t=0; t<input.threads.size(); t++) {
    tara::thread& thread = *threads[t];
    shared_ptr<hb_enc::location> prev;
    for (unsigned j=0; j<input.threads[t].size(); j++) {
      if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j])) {
        shared_ptr<hb_enc::location> loc = instr->location();
        locations.push_back(*loc);
        
        auto ninstr = make_shared<instruction>(_z3, loc, &thread, instr->name, instr->type, instr->instr);
        thread.add_instruction(ninstr);
      } else {
        throw cssa_exception("Instruction must be Z3");
      }
    }
  }
  
  // make a distinct attribute
  phi_distinct = distinct(locations);
  
  for(unsigned t=0; t<threads.size(); t++) {
    tara::thread& thread = *threads[t];
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

void cssa::program::build_pre(const input::program& input)
{
  if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.precondition)) {
    z3::expr_vector src(_z3.c);
    z3::expr_vector dst(_z3.c);
    for(const variable& v: instr->variables()) {
      variable nname(_z3.c);
      if (!is_primed(v)) {
        nname = v + "#pre";
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


cssa::program::program(z3interf& z3, hb_enc::encoding& hb_encoding, const input::program& input): tara::program(z3),
  _z3(z3), _hb_encoding(hb_encoding), globals(z3.translate_variables(input.globals))
{
  // add threads
  for (unsigned t=0; t<input.threads.size(); t++) {
    auto tp = make_shared<tara::thread>(z3, input.threads[t].name, z3.translate_variables(input.threads[t].locals));
    threads.push_back(move(tp));
  }
  
  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
  // set memory model
  set_mm( input.get_mm() );
  if( is_mm_declared() ) {
    wmm(input);
    //------------------------------------------------------------------------
    //end of wmm support
    //------------------------------------------------------------------------
  }else{
    build_hb(input);
    build_pre(input);
    build_threads(input);
  }

}

const tara::thread& cssa::program::operator[](unsigned int i) const
{
  return *threads[i];
}

unsigned int cssa::program::size() const
{
  return threads.size();
}

bool cssa::program::is_global(const variable& name) const
{
  return globals.find(variable(name))!=globals.end();
}

const tara::instruction& cssa::program::lookup_location(const hb_enc::location_ptr& location) const
{
  return (*this)[location->thread][location->instr_no];
}


std::vector< std::shared_ptr<const tara::instruction> > cssa::program::get_assignments_to_variable(const variable& variable) const
{
  string name = (get_unprimed(variable)).name;
  vector<std::shared_ptr<const instruction>> result;
  for (unsigned i = 0; i<this->size(); i++) {
    const tara::thread& t = (*this)[i];
    
    auto find = t.global_var_assign.find(name);
    if (find != t.global_var_assign.end()) {
      const auto& instrs = get<1>(*find);
      result.insert(result.end(), instrs.begin(), instrs.end());
    }
  }
  return result;
}

//----------------------------------------------------------------------------
// WMM support

// populate content of threads
void cssa::program::wmm_build_cssa_thread(const input::program& input) {

  for( unsigned t=0; t < input.threads.size(); t++ ) {
    tara::thread& thread = *threads[t];
    assert( thread.size() == 0 );

    shared_ptr<hb_enc::location> prev;
    for( unsigned j=0; j < input.threads[t].size(); j++ ) {
      if( shared_ptr<input::instruction_z3> instr =
          dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j]) ) {

        shared_ptr<hb_enc::location> loc = instr->location();
        auto ninstr = make_shared<instruction>( _z3, loc, &thread, instr->name,
                                                instr->type, instr->instr );
        thread.add_instruction(ninstr);
      }else{
        throw cssa_exception("Instruction must be Z3");
      }
    }
  }
}

// encode pre condition of multi-threaded code
void cssa::program::wmm_build_pre(const input::program& input) {
  //
  // start location is needed to ensure all locations are mentioned in phi_ppo
  //
  std::shared_ptr<hb_enc::location> _init_l = input.start_name();
  hb_enc::se_ptr wr = mk_se_ptr( _z3.c, _hb_encoding, threads.size(), 0,
                                 _init_l, hb_enc::event_kind_t::pre, se_store );
  wr->guard = _z3.c.bool_val(true);
  init_loc.insert(wr);
  for( const variable& v : globals ) {
    variable nname(_z3.c);
    nname = v + "#pre";
    initial_variables.insert(nname);
    wr_events[v].insert( wr );
  }

  if ( shared_ptr<input::instruction_z3> instr =
       dynamic_pointer_cast<input::instruction_z3>(input.precondition)) {
    z3::expr_vector src(_z3.c), dst(_z3.c);
    for( const variable& v: instr->variables() ) {
      variable nname(_z3.c);
      if (!is_primed(v)) {
        nname = v + "#pre";
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

void cssa::program::wmm_build_post(const input::program& input,
                             unordered_map<string, string>& thread_vars ) {
    
  if( shared_ptr<input::instruction_z3> instr =
      dynamic_pointer_cast<input::instruction_z3>(input.postcondition) ) {

    z3::expr tru = _z3.c.bool_val(true);
    if( eq( instr->instr, tru ) ) return;

    std::shared_ptr<hb_enc::location> _end_l = input.end_name();
    hb_enc::se_ptr rd = mk_se_ptr( _z3.c, _hb_encoding, threads.size(), INT_MAX,
                           _end_l, hb_enc::event_kind_t::post, se_store );
    rd->guard = _z3.c.bool_val(true);

    for( const variable& v : globals ) {
      variable nname(_z3.c);
      nname = v + "#post";
      post_loc.insert( rd );
      rd_events[v].push_back( rd );
    }

    z3::expr_vector src(_z3.c), dst(_z3.c);
    for( const variable& v: instr->variables() ) {
      variable nname(_z3.c);
      if( !is_primed(v) ) {
        if( is_global(v) ) {
          nname = v + "#post";
        }else{
          nname = v + "#" + thread_vars[v];
        }
      } else {
        throw cssa_exception("Primed variable in postcondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_post = instr->instr.substitute(src,dst);
    phi_prp = phi_prp && implies( phi_fea, phi_post );
    // todo : check the above implies is consistent with rest of the code
  } else {
    throw cssa_exception("Instruction must be Z3");
  }
}

void cssa::program::wmm_build_ssa( const input::program& input ) {

  wmm_build_pre( input );

  hb_enc::var_to_ses_map dep_events;
  hb_enc::var_to_ses_map ctrl_events;
  z3::context& c = _z3.c;

  unordered_map<string, string> thread_vars;

  for( unsigned t=0; t < input.threads.size(); t++ ) {
    // last reference to global variable in this thread (if any)
    unordered_map<string, shared_ptr<instruction> > global_in_thread;
    // maps variable to the last place they were assigned

    // set all local vars to "pre"
    for (string v: input.threads[t].locals) {
      thread_vars[ threads[t]->name + "." + v ] = "pre";
    }

    z3::expr path_constraint = c.bool_val(true);
    variable_set path_constraint_variables;
    tara::thread& thread = *threads[t];

    hb_enc::se_set ctrl_thread_ses;
    for ( unsigned i=0; i<input.threads[t].size(); i++ ) {
      if ( shared_ptr<input::instruction_z3> instr =
           dynamic_pointer_cast<input::instruction_z3>(input.threads[t][i]) ) {
        z3::expr_vector src(c), dst(c);
        hb_enc::se_set dep_ses;
        hb_enc::se_set ctrl_ses;
        // Construct ssa/symbolic events for all the read variables
        for( const variable& v1: instr->variables() ) {
          if( !is_primed(v1) ) {
            variable v = v1;
            variable nname(c);
            src.push_back(v);
            //unprimmed case
            if ( is_global(v) ) {
              nname =  "pi_"+ v + "#" + thread[i].loc->name;
              hb_enc::se_ptr rd = mk_se_ptr( c, _hb_encoding, t, i, nname, v,
                                     thread[i].loc, hb_enc::event_kind_t::r,se_store);
              thread[i].rds.insert( rd );
              rd_events[v].push_back( rd );
              dep_ses.insert( rd );
	      // if (thread[i].type==instruction_type::assume || thread[i].type==instruction_type::assert) {
	      //   ctrl_ses.insert( rd );
	      // }
              ctrl_dependency[rd].insert( ctrl_thread_ses.begin(),
                                                   ctrl_thread_ses.end()  );
            }else{
              v = thread.name + "." + v;
              nname = v + "#" + thread_vars[v];
              // check if we are reading an initial value
              if (thread_vars[v] == "pre") { initial_variables.insert(nname); }
              dep_ses.insert( dep_events[v].begin(), dep_events[v].end());
	      // ctrl_ses.insert( ctrl_events[v].begin(), ctrl_events[v].end());
            }
            // the following variable is read by this instruction
            thread[i].variables_read.insert(nname);
            thread[i].variables_read_orig.insert(v);
            dst.push_back(nname);
          }
        }
        // something gets added only if the instruction is asssume or assert
        // ctrl_thread_ses.insert( ctrl_ses.begin(), ctrl_ses.end() );
        if (thread[i].type==instruction_type::assume || thread[i].type==instruction_type::assert) {
          ctrl_thread_ses.insert( dep_ses.begin(), dep_ses.end() );
        }
        // Construct ssa/symbolic events for all the write variables
        for( const variable& v: instr->variables() ) {
          if( is_primed(v) ) {
            src.push_back(v);
            variable nname(c);
            //primmed case
            variable v1 = get_unprimed(v); //converting the primed to unprimed
            if( is_global(v1) ) {
              nname = v1 + "#" + thread[i].loc->name;
              //insert write symbolic event
              hb_enc::se_ptr wr = mk_se_ptr( c,_hb_encoding, t, i, nname,v1,
                                     thread[i].loc, hb_enc::event_kind_t::w,se_store);
              thread[i].wrs.insert( wr );
              wr_events[v1].insert( wr );
              data_dependency[wr].insert( dep_ses.begin(),dep_ses.end() );
	      ctrl_dependency[wr].insert( ctrl_thread_ses.begin(),
                                                   ctrl_thread_ses.end() );
            }else{
              v1 = thread.name + "." + v1;
              nname = v1 + "#" + thread[i].loc->name;
              dep_events[v1].insert( dep_ses.begin(), dep_ses.end() );
            }
            // the following variable is written by this instruction
            thread[i].variables_write.insert( nname );
            thread[i].variables_write_orig.insert( v1 );
            // map the instruction to the variable
            variable_written[nname] = thread.instructions[i];
            dst.push_back(nname);
          }
        }

        // thread havoced variables the same
        for( const variable& v: instr->havok_vars ) {
          assert(false); // untested code //todo: havoc ctrl dependency?
          assert( dep_ses.empty() ); // there must be nothing in dep_ses
          src.push_back(v);
          variable nname(c);
          variable v1 = get_unprimed(v);
          thread[i].havok_vars.insert(v1);
          if( is_global(v1) ) {
            nname = v1 + "#" + thread[i].loc->name;
            hb_enc::se_ptr wr = mk_se_ptr( c, _hb_encoding, t, i, nname, v1,
                                   thread[i].loc,hb_enc::event_kind_t::w, se_store);
            thread[i].wrs.insert( wr );
            wr_events[v1].insert( wr );
            data_dependency[wr].insert( dep_ses.begin(),dep_ses.end() );
            // ctrl_dependency[wr].insert( dep_ses.begin(),dep_ses.end() );
          }else{
            v1 = thread.name + "." + v1;
            nname = v1 + "#" + thread[i].loc->name;
            dep_events[v1].insert(dep_ses.begin(),dep_ses.end());
          }
          // this variable is written by this instruction
          thread[i].variables_write.insert(nname);
          thread[i].variables_write_orig.insert(v1);
          // map the instruction to the variable
          variable_written[nname] = thread.instructions[i];
          dst.push_back(nname);
          // add havok to initial variables
          initial_variables.insert(nname);
        }

        for( hb_enc::se_ptr rd : thread[i].rds ) {
          rd->guard = path_constraint;
        }

        // replace variables in expr
        z3::expr newi = instr->instr.substitute( src, dst );
        // deal with the path-constraint
        thread[i].instr = newi;
        if (thread[i].type == instruction_type::assume) {
          path_constraint = path_constraint && newi;
          path_constraint_variables.insert( thread[i].variables_read.begin(),
                                            thread[i].variables_read.end() );
        }
        thread[i].path_constraint = path_constraint;
        thread[i].variables_read.insert( path_constraint_variables.begin(),
                                         path_constraint_variables.end() );
        if ( instr->type == instruction_type::regular ) {
          phi_vd = phi_vd && newi;
        }else if ( instr->type == instruction_type::assert ) {
          // add this assertion, but protect it with the path-constraint
          phi_prp = phi_prp && implies(path_constraint,newi);
          // add used variables
          assertion_instructions.insert(thread.instructions[i]);
        }
        if( is_barrier( instr->type) ) {
          //todo : prepage contraints for barrier
          hb_enc::se_ptr barr = mk_se_ptr( c, _hb_encoding, t, i,
                                   thread[i].loc, hb_enc::event_kind_t::barr,se_store);
          thread[i].barr.insert( barr );
          tid_to_instr.insert({t,i}); // for shikhar code
        }
        for( hb_enc::se_ptr wr : thread[i].wrs ) {
          wr->guard = path_constraint;
        }

        // increase referenced variables
        for( variable v: thread[i].variables_write_orig ) {
          thread_vars[v] = thread[i].loc->name;
          if ( is_global(v) ) {
            global_in_thread[v] = thread.instructions[i]; // useless??
            thread.global_var_assign[v].push_back(thread.instructions[i]); //useless??
          }
        }
      }else{
        throw cssa_exception("Instruction must be Z3");
      }
    }
    phi_fea = phi_fea && path_constraint; //recordes feasibility of threads
  }
  wmm_build_post( input, thread_vars );
}

void cssa::program::wmm( const input::program& input ) {
  wmm_build_cssa_thread( input ); // construct thread skeleton
  wmm_build_ssa( input ); // build ssa

  //TODO: deal with atomic section and prespecified happens befores
  // add atomic sections
  for (input::location_pair as : input.atomic_sections) {
    cerr << "#WARNING: atomic sections are declared!! Not supproted in wmm mode!\n";
    // throw cssa_exception( "atomic sections are not supported!" );
  }
}

bool cssa::program::has_barrier_in_range( unsigned tid, unsigned start_inst_num,
                                    unsigned end_inst_num ) const {
  const tara::thread& thread = *threads[tid];
  for(unsigned i = start_inst_num; i <= end_inst_num; i++ ) {
    if( is_barrier( thread[i].type ) ) return true;
  }
  return false;
}


//----------------------------------------------------------------------------
// To be deleted
//----------------------------------------------------------------------------

// bool cssa::program::is_mm_declared() const {  return mm != mm_t::none; }
// bool cssa::program::is_wmm()         const {  return mm != mm_t::sc;   }
// bool cssa::program::is_mm_sc()       const {  return mm == mm_t::sc;   }
// bool cssa::program::is_mm_tso()      const {  return mm == mm_t::tso;  }
// bool cssa::program::is_mm_pso()      const {  return mm == mm_t::pso;  }
// bool cssa::program::is_mm_rmo()      const {  return mm == mm_t::rmo;  }
// bool cssa::program::is_mm_alpha()    const {  return mm == mm_t::alpha;}
// bool cssa::program::is_mm_power()    const {  return mm == mm_t::power;}

// mm_t cssa::program::get_mm()         const { return mm; }
// void cssa::program::set_mm(mm_t _mm)       { mm = _mm;  }

// void cssa::program::unsupported_mm() const {
//   std::string msg = "unsupported memory model: " + string_of_mm( mm ) + "!!";
//   throw cssa_exception( msg.c_str() );
// }

// void cssa::program::wmm_event_cons() {
  // wmm_mk_distinct_events(); // Rd/Wr events on globals are distinct
  // wmm_build_ppo(); // build hb formulas to encode the preserved program order
  // wmm_build_ses(); // build symbolic event structure
  // wmm_build_barrier(); // build barrier// ppo already has the code
// }

// unsigned cssa::program:: no_of_instructions(unsigned tid) const
// {
// 	return threads[tid]->instructions.size();
// }

// std::string cssa::program::instr_name(unsigned tid, unsigned instr_no) const
// {
// 	return threads[tid]->instructions[instr_no]->loc->name;
// }

// unsigned cssa::program::total_instructions() const
// {
// 	unsigned count=0;
// 	for(unsigned i=0; i < threads.size(); i++)
// 	{
// 		count+=no_of_instructions(i);
// 	}
// 	return count;
// }

// std::vector< vector < bool > > cssa::program::build_po() const
// {
// 	unsigned count=0,temp=0;
// 	for (unsigned t=0; t<threads.size(); t++)
// 	{
// 		thread& thread = *threads[t];
// 		count+=thread.size();
// 	}

// 	std::vector < std::vector <bool> > Adjacency_Matrix(count,std::vector <bool>(count,0));

// 	for (unsigned t=0; t<threads.size(); t++)
// 	{
// 		thread& thread = *threads[t];
// 		for (unsigned i=0; i<thread.size(); i++)
// 		{
// 			if(i!=thread.size()-1)
// 			{
// 				Adjacency_Matrix[temp][temp+1]=true;
// 			}
// 			temp=temp+1;
// 		}
// 	}
// 	return Adjacency_Matrix;
// }
