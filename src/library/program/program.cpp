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
    o << "var_read : ";      helpers::print_set( o, variables_read       );
    o << "var_write : ";     helpers::print_set( o, variables_write      );
    o << "var_read_orig : "; helpers::print_set( o, variables_read_orig  );
    o << "var_write_orig : ";helpers::print_set( o, variables_write_orig );
    o << "var_rds : ";       tara::debug_print(o, rds);
    o << "var_wrs : ";       tara::debug_print(o, wrs);
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
  unsigned int thread::events_size() const { return events.size(); }

  const instruction& thread::operator[](unsigned int i) const {
    return *instructions[i];
  }

  instruction& thread::operator[](unsigned int i) {
    return *instructions[i];
  }

  void thread::add_instruction(const std::shared_ptr<instruction>& instr) {
    instructions.push_back(instr);
  }

  void thread::update_post_events() {
    for( hb_enc::se_ptr& e : events ) {
      const auto& e_h= e->history;
      for( const hb_enc::se_ptr& ep : e->prev_events ) {
        const auto& ep_h = ep->history;
        unsigned min_size = std::min( e_h.size(), ep_h.size() );
        unsigned i =0;
        for(; i < min_size; i++  ) {
          if( !z3::eq( e_h[i],ep_h[i] ) ) {
            break;
          }
        }
        ep->add_post_events( e, e->guard ); // todo : wrong code
      }
    }
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

  void program::update_post_events() {
    for( auto& t : threads ) {
      t->update_post_events();
    }
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

  std::ostream& operator << (std::ostream& os, const hb_enc::depends_set& dep_ses) {
    for( auto iter = dep_ses.begin(); iter!=dep_ses.end(); ++iter ) {
      hb_enc::depends element = *iter;
      hb_enc::se_ptr val = element.e;
      z3::expr condition = element.cond;
      os << "      |" << val->name() << "|" << " => " << "[condition = " <<  condition << "]" << std::endl;
    }
    return os;
  }

  void program::print_dependency( std::ostream& os ) {
      for (unsigned t=0; t<threads.size(); t++) {
        auto& thread = *threads[t];
        for( const auto& e : thread.events ) {
	  hb_enc::depends_set& dep_ses = e->data_dependency;
	  os << "data" << "|" << e->name() << "|=>\n" << dep_ses << std::endl;
        }
      }
      for (unsigned t=0; t<threads.size(); t++) {
        auto& thread = *threads[t];
        for( const auto& e : thread.events ) {
          hb_enc::depends_set& ctrl_ses = e->ctrl_dependency;
          os << "ctrl" << "|" << e->name() << "|=>\n" << ctrl_ses << std::endl;
        }
      }
  }

  void program::print_dot( std::ostream& os ) {
    os << "digraph prog {" << std::endl;
    print_node( os, init_loc );
    if( post_loc ) print_node( os, post_loc );
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
        
        print_edge( os, thread.final_event, join_map.at(thread.name).first,
                    "brown" );
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

  void program::gather_statistics( api::metric& metric ) {
    metric.threads = size();
    for(unsigned i = 0; i < size(); i++) {
      metric.instructions += (threads)[i]->events_size();
    }
  }


  void program::wmm_print_dot( std::ostream& stream, z3::model m ) const {
    // output the program as a dot file
    stream << "digraph hbs {" << std::endl;
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
        stream << "]"<< std::endl;
      }
      stream << " }\n";
    }

    stream << "pre" << " [label=\""  << phi_pre << "\"]" << std::endl;
    stream << "post" << " [label=\"" << phi_post << "\"]" << std::endl;

    // add white edges between instructions
    for (unsigned t=0; t<threads.size(); t++) {
      auto& thread = *threads[t];
      if( thread.instructions.size() > 0 ) {
        stream << "pre ->" << "\"" << thread[0].loc->name << "\""
               << " [color=white]" << std::endl;
      }
      for (unsigned i=0; i<thread.instructions.size()-1; i++) {
        stream << "\"" << thread[i].loc->name << "\""  << "->"
               << "\"" << thread[i+1].loc->name << "\""
               << " [color=white]" << std::endl;
      }
      if( thread.instructions.size() > 0 ) {
        stream << "\"" << thread[thread.instructions.size()-1].loc->name << "\""
               << "-> post [color=white]" << std::endl;
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
               << "[color=blue]"<< std::endl;
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
                       << "[color=orange,constraint=false]"<< std::endl;
              }
            }
          }
        }
      }
    }

    for(const cssa::variable& v1 : globals ) {
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
               << "[color=green]" <<  std::endl;
      }
    }

    // for (hb_enc::hb hb : hbs) {
    //   stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << std::endl;
    // }
    stream << "}" << std::endl;

  }
}
