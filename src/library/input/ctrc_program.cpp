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


#include <iostream>
#include <fstream>

#include "program/program.h"
#include "input/ctrc_program.h"
#include "helpers/helpers.h"
#include <vector>
using namespace tara;
using namespace tara::cssa;
using namespace tara::ctrc;
using namespace tara::helpers;

using namespace std;

pi_function_part::pi_function_part(variable_set variables, z3::expr hb_exression) : variables(variables), hb_exression(hb_exression)
{}

pi_function_part::pi_function_part(z3::expr hb_exression) : hb_exression(hb_exression)
{}

void ctrc::program::build_threads(const input::program& input)
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
        for(const auto& v: instr->variables()) {
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
            auto v1 = get_unprimed(v);
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
        ctrc_input_error( "Instruction must be Z3" );
      }
    }
    phi_fea = phi_fea && path_constraint;
  }
  build_pis(pis, input);
}

//todo: move the following function in cssa folder
void ctrc::program::build_pis(vector< ctrc::program::pi_needed >& pis, const input::program& input)
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


void ctrc::program::build_hb(const input::program& input)
{
  z3::expr_vector locations(_z3.c);
  // start location is needed to ensure all locations are mentioned in phi_po
  hb_enc::tstamp_ptr start_location = input.start_name();
  locations.push_back(*start_location);
  
  for (unsigned t=0; t<input.threads.size(); t++) {
    tara::thread& thread = *threads[t];
    for (unsigned j=0; j<input.threads[t].size(); j++) {
      if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j])) {
        hb_enc::tstamp_ptr loc = instr->location();
        locations.push_back(*loc);
        
        auto ninstr = make_shared<instruction>(_z3, loc, &thread, instr->name, instr->type, instr->instr);
        thread.add_instruction(ninstr);
      } else {
        ctrc_input_error( "Instruction must be Z3" );
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
    hb_enc::tstamp_ptr loc1 = _hb_encoding.get_tstamp(get<0>(as));
    hb_enc::tstamp_ptr loc2 = _hb_encoding.get_tstamp(get<1>(as));
    phi_po = phi_po && _hb_encoding.make_as(loc1, loc2);
  }
  // add happens-befores
  for (input::location_pair hb : input.happens_befores) {
    hb_enc::tstamp_ptr loc1 = _hb_encoding.get_tstamp(get<0>(hb));
    hb_enc::tstamp_ptr loc2 = _hb_encoding.get_tstamp(get<1>(hb));
    phi_po = phi_po && _hb_encoding.make_hb(loc1, loc2);
  }
  phi_po = phi_po && phi_distinct;
}

void ctrc::program::build_pre(const input::program& input)
{
  if (shared_ptr<input::instruction_z3> instr = dynamic_pointer_cast<input::instruction_z3>(input.precondition)) {
    z3::expr_vector src(_z3.c);
    z3::expr_vector dst(_z3.c);
    for(const variable& v: instr->variables()) {
      variable nname(_z3.c);
      if (!is_primed(v)) {
        nname = v + "#pre";
      } else {
        ctrc_input_error("Primed variable in precondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_pre = instr->instr.substitute(src,dst);
  } else {
    ctrc_input_error("Instruction must be Z3");
  }
}



void ctrc::program::print_hb(const z3::model& m, ostream& out, bool machine_readable) const
{  if( is_mm_declared() ) { wmm_print_dot( m ); return; } //wmm support
  if (!machine_readable){
    // initial values
    auto git = initial_variables.begin();
    if (git!=initial_variables.end()) {
      out << "init: [ ";
      bool first = true;
      for(; git!=initial_variables.end(); git++) {
        if (!first) {
          out << "; ";
        }
        first = false;
        out << (string)*git << "=" << m.eval(*git);
      }
      out << " ]" << endl;
    }
  }
  
  // get into right order
  vector<instruction> ordered;
  for (unsigned t=0; t<threads.size(); t++) {
    auto& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size(); i++) {
      std::vector<instruction>::iterator it;
      for (it = ordered.begin() ; it != ordered.end(); ++it) {
        if (!_hb_encoding.eval_hb(m, it->loc, thread[i].loc)) {
          // insert here when the new instruction is not after *it
          break;
        }
      }
      ordered.insert(it, thread[i]);
      // terminate if assumption fails
      if (thread[i].type==instruction_type::assume && !m.eval(thread[i].instr).get_bool())
        break;
    }
  }
  
  // output with variables
  for (instruction i: ordered) {
    if (!machine_readable) {
      out << i;
      auto vit = i.variables_write.begin();
      if (vit!=i.variables_write.end()) {
        out << " [ ";
        bool first = true;
        for(; vit!=i.variables_write.end(); vit++) {
          if (!first) {
            out << "; ";
          }
          first = false;
          out << get_before_hash(*vit) << ".=" << m.eval(*vit);
        }
        out << " ]";
      }
      // if this is a failing assumption
      if (i.type==instruction_type::assume || i.type==instruction_type::assert)
        if (!m.eval(i.instr).get_bool())
          out << " (fails)";
      out << endl;
    } else {
      out << i.loc << " ";
    }
  }
  out << endl;
}

void ctrc::program::print_dot(ostream& stream, vector< hb_enc::hb >& hbs) const
{
  // output the program as a dot file
  stream << "digraph hbs {" << endl;
  // output labels
  for (unsigned t=0; t<threads.size(); t++) {
    auto& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size(); i++) {
      stream << thread[i].loc->name << "[label=\"" << thread[i] << "\"]" << endl;
    }
  }
  // add white edges between instructions
  for (unsigned t=0; t<threads.size(); t++) {
    auto& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size()-1; i++) {
      stream << thread[i].loc->name << "->" << thread[i+1].loc->name << " [color=white]" << endl;
    }
  }
  for (hb_enc::hb hb : hbs) {
    stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << endl;
  }
  stream << "}" << endl;
}


ctrc::program::program(z3interf& z3, api::options& o, hb_enc::encoding& hb_encoding, const input::program& input): tara::program(z3,o, hb_encoding)
{
  prog_type = prog_t::original;// when is it ctrc // todo: ctrc type
  globals = z3.translate_variables(input.globals);
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

//----------------------------------------------------------------------------
// ctrc WMM support

// populate content of threads
void ctrc::program::wmm_build_cssa_thread(const input::program& input) {

  for( unsigned t=0; t < input.threads.size(); t++ ) {
    tara::thread& thread = *threads[t];
    assert( thread.size() == 0 );

    for( unsigned j=0; j < input.threads[t].size(); j++ ) {
      if( shared_ptr<input::instruction_z3> instr =
          dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j]) ) {

        hb_enc::tstamp_ptr loc = instr->location();
        auto ninstr = make_shared<instruction>( _z3, loc, &thread, instr->name,
                                                instr->type, instr->instr );
        thread.add_instruction(ninstr);
      }else{
        ctrc_input_error("Instruction must be Z3");
      }
    }
  }
}

// encode pre condition of multi-threaded code
void ctrc::program::wmm_build_pre(const input::program& input) {
  //
  // start location is needed to ensure all locations are mentioned in phi_ppo
  //
  std::string _init_l = input.start_name()->name;
  hb_enc::se_set last;
  hb_enc::se_ptr wr = mk_se_ptr_old( _hb_encoding, threads.size(), 0,
                                     _init_l, hb_enc::event_t::pre, last);
  add_event( threads.size(), wr );
  wr->guard = _z3.c.bool_val(true);
  init_loc = wr;
  for( unsigned t = 0; t < size(); t++ ) create_map[ threads[t]->name ] = wr;
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
        ctrc_input_error("Primed variable in precondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_pre = instr->instr.substitute(src,dst);
  } else {
    ctrc_input_error("Instruction must be Z3");
  }
}

void ctrc::program::wmm_build_post(const input::program& input,
                                   unordered_map<string, string>& thread_vars,
                                   hb_enc::se_set last // not needed
                                   ) {
    
  if( shared_ptr<input::instruction_z3> instr =
      dynamic_pointer_cast<input::instruction_z3>(input.postcondition) ) {

    z3::expr tru = _z3.c.bool_val(true);
    if( eq( instr->instr, tru ) ) return;

    std::string _end_l = input.end_name()->name;
    hb_enc::se_ptr rd = mk_se_ptr_old( _hb_encoding, threads.size(), INT_MAX,
                                       _end_l,hb_enc::event_t::post, last);
    rd->guard = _z3.mk_true();//c.bool_val(true);
    add_event( threads.size(), rd);
    for( unsigned t = 0; t < size(); t++ )
      join_map.insert(std::make_pair(threads[t]->name, std::make_pair(rd,tru)));
      // join_map[ threads[t]->name ] = rd;

    for( const variable& v : globals ) {
      variable nname(_z3.c);
      nname = v + "#post";
      post_loc = rd;
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
        ctrc_input_error("Primed variable in postcondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_post = instr->instr.substitute(src,dst);
    phi_prp = phi_prp && implies( phi_fea, phi_post );

    // todo : check the above implies is consistent with rest of the code
  } else {
    ctrc_input_error("Instruction must be Z3");
  }
}

void ctrc::program::wmm_build_ssa( const input::program& input ) {

  wmm_build_pre( input );

  hb_enc::var_to_depends_map dep_events;
  // hb_enc::var_to_depends_map ctrl_events;
  z3::context& c = _z3.c;

  unordered_map<string, string> thread_vars;
  hb_enc::se_set all_lasts;
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

    hb_enc::depends_set ctrl_ses;
    hb_enc::se_set last;// = init_loc;
    auto start = mk_se_ptr_old( _hb_encoding, t, 0, "start__"+threads[t]->name,
                                hb_enc::event_t::barr, last);
    set_start_event( t, start, _z3.mk_true() );
    last.insert( start );
    for ( unsigned i=0; i<input.threads[t].size(); i++ ) {
      if ( shared_ptr<input::instruction_z3> instr =
           dynamic_pointer_cast<input::instruction_z3>(input.threads[t][i]) ) {
        z3::expr_vector src(c), dst(c);
        hb_enc::depends_set dep_ses;
        // Construct ssa/symbolic events for all the read variables
        for( const variable& v1: instr->variables() ) {
          if( !is_primed(v1) ) {
            variable v = v1, nname(c);
            src.push_back(v);
            //unprimmed case
            if ( is_global(v) ) {
              hb_enc::se_ptr rd = mk_se_ptr_old( _hb_encoding, t, i, v,
                                                 thread[i].loc->name,
                                                 hb_enc::event_t::r, last);
              add_event( t, rd );
              nname = rd->v;
              thread[i].rds.insert( rd );
              // rd_events[v].push_back( rd ); //to be commented
              dep_ses.insert( hb_enc::depends( rd, _z3.mk_true() ) );
              rd->ctrl_dependency.insert( ctrl_ses.begin(), ctrl_ses.end()  );
            }else{
              v = thread.name + "." + v;
              nname = v + "#" + thread_vars[v];
              // check if we are reading an initial value
              if (thread_vars[v] == "pre") { initial_variables.insert(nname); }
              dep_ses.insert( dep_events[v].begin(), dep_events[v].end());
            }
            // the following variable is read by this instruction
            thread[i].variables_read.insert(nname);
            thread[i].variables_read_orig.insert(v);
            dst.push_back(nname);
          }
        }
        if( thread[i].rds.size() > 0 ) last = thread[i].rds;

        // something gets added only if the instruction is asssume or assert
        if( thread[i].type==instruction_type::assume ||
            thread[i].type==instruction_type::assert ) {
          ctrl_ses.insert( dep_ses.begin(), dep_ses.end() );
        }
        // Construct ssa/symbolic events for all the write variables
        for( const variable& v: instr->variables() ) {
          if( is_primed(v) ) {
            src.push_back(v);
            variable nname(c);
            //primmed case
            variable v1 = get_unprimed(v); //converting the primed to unprimed
            if( is_global(v1) ) {
              //insert write symbolic event
              hb_enc::se_ptr wr = mk_se_ptr_old( _hb_encoding, t, i, v1,
                                                 thread[i].loc->name,
                                                 hb_enc::event_t::w, last);
              add_event( t, wr );
              nname = wr->v;
              thread[i].wrs.insert( wr );
              // wr_events[v1].insert( wr );//to be commented
              wr->data_dependency.insert( dep_ses.begin(),dep_ses.end() );
	      wr->ctrl_dependency.insert( ctrl_ses.begin(), ctrl_ses.end());
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
        if( thread[i].wrs.size() > 0 ) last = thread[i].wrs;

        // thread havoced variables the same
        for( const variable& v: instr->havok_vars ) {
          assert(false); // untested code //todo: havoc ctrl dependency?
          assert( dep_ses.empty() ); // there must be nothing in dep_ses
          src.push_back(v);
          variable nname(c);
          variable v1 = get_unprimed(v);
          thread[i].havok_vars.insert(v1);
          if( is_global(v1) ) {
            // nname = v1 + "#" + thread[i].loc->name;
            hb_enc::se_ptr wr = mk_se_ptr_old( _hb_encoding, t, i, v1,
                                               thread[i].loc->name,
                                               hb_enc::event_t::w, last);
            add_event( t, wr );
            nname = wr->v;
            thread[i].wrs.insert( wr );
            // wr_events[v1].insert( wr ); // to be commented
            wr->data_dependency.insert( dep_ses.begin(),dep_ses.end() );
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
          hb_enc::se_ptr barr = mk_se_ptr_old( _hb_encoding, t, i,
                                               thread[i].loc->name,
                                               hb_enc::event_t::barr,
                                               last );
          add_event( t, barr );
          thread[i].barr.insert( barr );
          last = thread[i].barr;
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
        ctrc_input_error("Instruction must be Z3");
      }
    }
    auto final = mk_se_ptr_old( _hb_encoding, t,INT_MAX,
                                "end__"+threads[t]->name,
                                hb_enc::event_t::barr, last);
    set_final_event( t, final, _z3.mk_true());
    all_lasts.insert( final );
    phi_fea = phi_fea && path_constraint; //recordes feasibility of threads
  }
  wmm_build_post( input, thread_vars, all_lasts );
}

void ctrc::program::wmm( const input::program& input ) {
  wmm_build_cssa_thread( input ); // construct thread skeleton
  wmm_build_ssa( input ); // build ssa

  //TODO: deal with atomic section and prespecified happens befores
  // add atomic sections
  for (input::location_pair as : input.atomic_sections) {
    cerr << "#WARNING: atomic sections are declared!! Not supproted in wmm mode!\n";
    // ctrc_input_error( "atomic sections are not supported!" );
  }
}

bool ctrc::program::has_barrier_in_range( unsigned tid, unsigned start_inst_num,
                                    unsigned end_inst_num ) const {
  const tara::thread& thread = *threads[tid];
  for(unsigned i = start_inst_num; i <= end_inst_num; i++ ) {
    if( is_barrier( thread[i].type ) ) return true;
  }
  return false;
}

//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------

void ctrc::program::wmm_print_dot( z3::model m ) const {
  boost::filesystem::path fname = _o.output_dir;
  fname += ".iteration-sat-dump";
  std::cerr << "dumping dot file : " << fname << std::endl;
  std::ofstream myfile;
  // std::string fname =  (std::string)(_o.output_dir) + ;
  myfile.open( fname.c_str() );
  if( myfile.is_open() ) {
    wmm_print_dot( myfile, m );
  }else{
    cssa_error( "failed to open " << fname.c_str() );
  }
  myfile.close();
}

void ctrc::program::wmm_print_dot( ostream& stream, z3::model m ) const {
    // output the program as a dot file
  stream << "digraph hbs {" << endl;
  // output labels

  for (unsigned t=0; t<threads.size(); t++) {
    auto& thread = *threads[t];
    stream << "subgraph cluster_" << t << " {\n";
    stream << "color=lightgrey;\n";
    stream << "label = \"" << thread.name << "\"\n";
    for (unsigned i=0; i < thread.instructions.size(); i++) {
      stream << "\"" << thread[i].loc->name << "\""
             << " [label=\"" << thread[i] << "\"";
      z3::expr v = m.eval( thread[i].path_constraint );
      if( Z3_get_bool_value( v.ctx(), v)  != Z3_L_TRUE) {
        stream << ",color=gray";
      }
      stream << "]"<< endl;
    }
    stream << " }\n";
  }

  stream << "pre" << " [label=\""  << phi_pre << "\"]" << endl;
  stream << "post" << " [label=\"" << phi_post << "\"]" << endl;

  // add white edges between instructions
  for (unsigned t=0; t<threads.size(); t++) {
    auto& thread = *threads[t];
    if( thread.instructions.size() > 0 ) {
      stream << "pre ->" << "\"" << thread[0].loc->name << "\""
             << " [color=white]" << endl;
    }
    for (unsigned i=0; i<thread.instructions.size()-1; i++) {
      stream << "\"" << thread[i].loc->name << "\""  << "->"
             << "\"" << thread[i+1].loc->name << "\""
             << " [color=white]" << endl;
    }
    if( thread.instructions.size() > 0 ) {
      stream << "\"" << thread[thread.instructions.size()-1].loc->name << "\""
             << "-> post [color=white]" << endl;
    }
  }

  for( auto& it : reading_map ) {
    std::string bname = std::get<0>(it);
    z3::expr b = _z3.c.bool_const(  bname.c_str() );
    z3::expr bv = m.eval( b );
    if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
      hb_enc::se_ptr wr = std::get<1>(it);
      hb_enc::se_ptr rd = std::get<2>(it);
      unsigned wr_tid      = wr->e_v->thread;
      std::string wr_name;
      if( wr_tid == threads.size() ) {
        wr_name = "pre";
      }else{
        auto& wr_thread = *threads[wr_tid];
        unsigned wr_instr_no = wr->e_v->instr_no;
        wr_name = wr_thread[wr_instr_no].loc->name;
      }
      unsigned rd_tid      = rd->e_v->thread;
      std::string rd_name;
      if( rd_tid == threads.size() ) {
        rd_name = "post";
      }else{
        auto& rd_thread = *threads[rd_tid];
        unsigned rd_instr_no = rd->e_v->instr_no;
        rd_name = rd_thread[rd_instr_no].loc->name;
      }
      stream << "\"" << wr_name  << "\" -> \"" << rd_name << "\""
             << "[color=blue]"<< endl;
      //fr
      std::set< hb_enc::se_ptr > fr_wrs;
      auto it = wr_events.find(rd->prog_v);
      if( it != wr_events.end() ) { // fails for dummy variable
        for( const auto& after_wr : it->second ) {
          z3::expr v = m.eval( after_wr->guard );
          if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE) {
            if( _hb_encoding.eval_hb( m, wr, after_wr ) ) {
              unsigned after_wr_tid      = after_wr->e_v->thread;
              auto& after_wr_thread = *threads[after_wr_tid];
              unsigned after_wr_instr_no = after_wr->e_v->instr_no;
              std::string after_wr_name =
                after_wr_thread[after_wr_instr_no].loc->name;
              stream << "\"" << rd_name  << "\" -> \"" << after_wr_name << "\""
                     << "[color=orange,constraint=false]"<< endl;
            }
          }
        }
      }
    }
  }

  for(const variable& v1 : globals ) {
    std::set< hb_enc::se_ptr > wrs;
    auto it = wr_events.find( v1 );
    for( const auto& wr : it->second ) {
      z3::expr v = m.eval( wr->guard );
      if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE)
        wrs.insert( wr );
    }
    hb_enc::se_vec ord_wrs;
    while( !wrs.empty() ) {
      hb_enc::se_ptr min;
      for ( auto wr : wrs ) {
        if( min ) {
          if( _hb_encoding.eval_hb( m, wr, min ) ) {
            min = wr;
          }
        }else{
          min = wr;
        }
      }
      ord_wrs.push_back(min);
      wrs.erase( min );
    }

    for( unsigned i = 1; i < ord_wrs.size(); i++  ) {
      unsigned wr_tid      = ord_wrs[i-1]->e_v->thread;
      std::string wr_name;
      if( wr_tid == threads.size() ) {
        wr_name = "pre";
      }else{
        auto& wr_thread = *threads[wr_tid];
        unsigned wr_instr_no = ord_wrs[i-1]->e_v->instr_no;
        wr_name = wr_thread[wr_instr_no].loc->name;
      }

      unsigned wr1_tid = ord_wrs[i]->e_v->thread;
      unsigned wr1_instr_no = ord_wrs[i]->e_v->instr_no;
      assert( wr1_tid != threads.size() );
      auto& wr1_thread = *threads[wr1_tid];
      std::string wr1_name = wr1_thread[wr1_instr_no].loc->name;

      stream << "\"" << wr_name << "\" -> \"" << wr1_name << "\""
             << "[color=green]" <<  endl;
    }
  }

  if( is_mm_power() ) {
  //ppo
  	for( auto& it : ppo ) {
      z3::expr cond = m.eval( std::get<0>(it) );
      if( Z3_get_bool_value( cond.ctx(), cond) == Z3_L_TRUE ) {
        hb_enc::se_ptr e1 = std::get<1>(it);
        hb_enc::se_ptr e2 = std::get<2>(it);
        stream << "\"" << e1->name() << "\""  << "->"
            << "\"" << e2->name() << "\""
            << " [color=" << "purple" << "]" << std::endl;
      }
    }
    //prop_base
    //prop
    for( auto& it : prop ) {
      z3::expr cond = m.eval( it.second );
      if( Z3_get_bool_value( cond.ctx(), cond) == Z3_L_TRUE ) {
        hb_enc::se_ptr e1 = it.first.first;
        hb_enc::se_ptr e2 = it.first.second;
        stream << "\"" << e1->name() << "\""  << "->"
               << "\"" << e2->name() << "\""
               << " [color=" << "cyan" << "]" << std::endl;
      }
    }
    //obs
  }
  // for (hb_enc::hb hb : hbs) {
  //   stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << endl;
  // }
  stream << "}" << endl;

}

//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------

