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

#include "helpers/helpers.h"
#include "program/program.h"
#include <fstream>

using namespace tara::helpers;


namespace tara {

instruction::instruction( z3interf& z3,
                          hb_enc::location_ptr location,
                          tara::thread* thread,
                          std::string& name,
                          instruction_type type,
                          z3::expr original_expression )
  : loc(location)
  , instr(z3::expr(z3.c))
  , path_constraint(z3::expr(z3.c))
  , in_thread(thread)
  , name(name)
  , type(type)
  , original_expr(original_expression) {
    if( thread == nullptr )
      program_error( "Thread cannot be null" );
}

std::ostream& operator <<(std::ostream& stream, const instruction& i) {
  stream << i.loc->name << ": ";
  if (i.havok_vars.size() > 0) {
    stream << "havok(";
    for (auto it = i.havok_vars.begin(); it!=i.havok_vars.end(); it++) {
      stream << *it;
      if (!helpers::last_element(it, i.havok_vars))
        stream << ", ";
    }
    stream << ") ";
  }
  switch (i.type) {
    case instruction_type::assert:
      stream << "assert " << i.original_expr;
      break;
    case instruction_type::assume:
      stream << "assume " << i.original_expr;
      break;
    default:
      stream << i.original_expr;
  }
  return stream;
}

  cssa::variable_set instruction::variables() const {
    return helpers::set_union(variables_read, variables_write);
  }

  cssa::variable_set instruction::variables_orig() const {
    return helpers::set_union(variables_read_orig, variables_write_orig);
  }

  void instruction::debug_print( std::ostream& o ) {
    o << (*this) << "\n";
    o << "ssa instruction : " << instr << "\n";
    o << "path constraint : " << path_constraint << "\n";
    o << "var_read : ";      helpers::print_set( variables_read,       o);
    o << "var_write : ";     helpers::print_set( variables_write,      o);
    o << "var_read_orig : "; helpers::print_set( variables_read_orig,  o);
    o << "var_write_orig : ";helpers::print_set( variables_write_orig, o);
    o << "var_rds : ";       hb_enc::debug_print_se_set(rds, o);
    o << "var_wrs : ";       hb_enc::debug_print_se_set(wrs, o);
  }

  //==========================================================================

  thread::thread( helpers::z3interf& z3_,
                  const std::string& name, const tara::cssa::variable_set locals)
    : z3(z3_)
    , name(name)
    , locals(locals)
  {}

  bool thread::operator==(const thread &other) const {
    return this->name == other.name;
  }

  bool thread::operator!=(const thread &other) const {
    return !(*this==other);
  }

  unsigned int thread::size() const { return instructions.size(); }
  const instruction& thread::operator[](unsigned int i) const {
    return *instructions[i];
  }

  instruction& thread::operator[](unsigned int i) {
    return *instructions[i];
  }

  void thread::add_instruction(const std::shared_ptr<instruction>& instr) {
    instructions.push_back(instr);
  }

  const tara::thread& program::operator[](unsigned int i) const {
    return *threads[i];
  }

  unsigned int program::size() const {
    return threads.size();
  }

  bool program::is_global(const cssa::variable& name) const
  {
    return globals.find(cssa::variable(name))!=globals.end();
  }

  const tara::instruction&
  program::lookup_location(const hb_enc::location_ptr& location) const {
    return (*this)[location->thread][location->instr_no];
  }

  void program::print_dot( const std::string& name ) {
    boost::filesystem::path fname = _o.output_dir;
    fname += "program-"+name+"-.dot";
    std::cerr << "dumping dot file : " << fname << std::endl;
    std::ofstream myfile;
    myfile.open( fname.c_str() );
    if( myfile.is_open() ) {
      print_dot( myfile );
    }else{
      program_error( "failed to open" << fname.c_str() );
    }
    myfile.close();
  }
  // local function
  void print_node( std::ostream& os,
                   const hb_enc::se_ptr& e,
                   std::string color = "" ) {
    assert( e );
    if( color == "" ) color = "black";
    os << "\"" << e->name() << "\"" << " [label=\"" << e->name()
       << "\",color=" << color << "]\n";
  }

  void print_edge( std::ostream& os,
                   const hb_enc::se_ptr& e1,
                   const hb_enc::se_ptr& e2,
                   std::string color = "" ) {
    assert( e1 );
    assert( e2 );
    if( color == "" ) color = "black";
    os << "\"" << e1->name() << "\""  << "->"
       << "\"" << e2->name() << "\""
       << " [color=" << color << "]" << std::endl;
  }

  void program::print_dot( std::ostream& os ) {
    os << "digraph prog {" << std::endl;
    print_node( os, *init_loc.begin() );
    if( post_loc.size() > 0 ) print_node( os, *post_loc.begin() );
    for (unsigned t=0; t<threads.size(); t++) {
      auto& thread = *threads[t];
      os << "subgraph cluster_" << t << " {\n";
      os << "color=lightgrey;\n";
      os << "label = \"" << thread.name << "\"\n";
      print_node( os, thread.start_event );
      print_node( os, thread.final_event );
      for( const auto& e : thread.events ) {
        print_node( os , e );
        for( const auto& ep : e->prev_events ) {
          print_edge( os, ep , e, "brown" );
        }
      }
      for ( const auto& ep : thread.final_event->prev_events ) {
        print_edge( os, ep, thread.final_event, "brown" );
      }
      os << " }\n";
      if( create_map.find( thread.name ) != create_map.end() ) {
        print_edge( os, create_map[thread.name], thread.start_event, "brown" );
      }
      if( join_map.find( thread.name ) != join_map.end() ) {
        print_edge( os, thread.final_event, join_map[thread.name], "brown" );
      }
    }
    os << "}" << std::endl;
  }

  z3::expr program::get_initial(const z3::model& m) const {
    z3::expr res = _z3.c.bool_val(true);
    for( auto v: initial_variables ) {
      z3::expr vname = v;
      z3::expr e = m.eval(vname);
      if (((Z3_ast)vname) != ((Z3_ast)e))
        res = res && vname == e;
    }
  return res;
}


}
