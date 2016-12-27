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

  void program::add_event( unsigned i, hb_enc::se_ptr e )   {
    if( i < threads.size() ) {
      threads[i]->add_event( e );
      if( e->is_rd() ) rd_events[e->prog_v].push_back( e );
      if( e->is_wr() ) wr_events[e->prog_v].insert( e );
      if( e->is_sc() ) all_sc.insert( e );
    }
    // todo what to do with pre/post events
    se_store[e->name()] = e;
  }

  void program::set_c11_rs_heads( hb_enc::se_ptr e,
                                  std::map< cssa::variable,
                                            hb_enc::depends_set >& rs_heads_map ) {
    //for read events there are no relase heads
    if( is_mm_c11() && e->is_wr() ) {
      hb_enc::depends_set& rs_heads = rs_heads_map[e->prog_v];
      // todo : ability to throw exception if a new model is seen
      if( e->is_at_least_rls() ) {
        rs_heads.clear(); rs_heads.insert(hb_enc::depends( e, _z3.mk_true() ));
      }
      c11_rs_heads[e] = rs_heads;
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

  std::vector< std::shared_ptr<const tara::instruction> >
  program::get_assignments_to_variable(const cssa::variable& variable) const
  {
    assert( prog_type == prog_t::original );
    std::string name = (get_unprimed(variable)).name;
    std::vector<std::shared_ptr<const instruction>> result;
    for (unsigned i = 0; i<this->size(); i++) {
      const tara::thread& t = (*this)[i];

      auto find = t.global_var_assign.find(name);
      if (find != t.global_var_assign.end()) {
        const auto& instrs = std::get<1>(*find);
        result.insert(result.end(), instrs.begin(), instrs.end());
      }
    }
    return result;
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
    os << "============================\n";
    os << "data dependencies:\n";
    os << "============================\n";
    for (unsigned t=0; t<threads.size(); t++) {
      auto& thread = *threads[t];
      for( const auto& e : thread.events ) {
        if( e->is_mem_op() ) {
          hb_enc::depends_set& dep_ses = e->data_dependency;
          os << e->name() << "=>\n" << dep_ses << std::endl;
        }
      }
    }
    os << "============================\n";
    os << "ctrl dependencies:\n";
    os << "============================\n";
    for (unsigned t=0; t<threads.size(); t++) {
      auto& thread = *threads[t];
      for( const auto& e : thread.events ) {
        if( e->is_mem_op() ) {
          hb_enc::depends_set& ctrl_ses = e->ctrl_dependency;
          os << e->name() << " =>\n" << ctrl_ses << std::endl;
        }
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
      if( is_original() )
        metric.instructions += (threads)[i]->size();
      else
        metric.instructions += (threads)[i]->events_size();
    }
    if( is_original() ) {
      //todo: we should add more stats
      //   // count pi functions
      //   for (auto pi : pi_functions) {
      //     metric.shared_reads++;
      //     metric.sum_reads_from += get<1>(pi).size();
      //   }
      // std::cerr << "Warning: pi function counting has been removed programatic reasons!!";
    }
  }



  void program::print_execution( const std::string& name, z3::model m ) const {
    boost::filesystem::path fname = _o.output_dir;
    fname += "program-execution-"+name+"-.dot";
    std::cerr << "dumping dot file : " << fname << std::endl;
    std::ofstream myfile;
    myfile.open( fname.c_str() );
    if( myfile.is_open() ) {
      print_execution( myfile, m );
    }else{
      program_error( "failed to open" << fname.c_str() );
    }
    myfile.close();
  }


  bool compare_events( std::pair< hb_enc::se_ptr, int >& a,
                       std::pair< hb_enc::se_ptr, int >& b ) {
    return a.second < b.second;
  }

  void program::print_execution( std::ostream& stream, z3::model m ) const {
    // output the program as a dot file
    stream << "digraph hbs {" << std::endl;
    // output labels

    for (unsigned t=0; t<threads.size(); t++) {
      auto& thread = *threads[t];
      stream << "subgraph cluster_" << t << " {\n";
      stream << "color=lightgrey;\n";
      stream << "label = \"" << thread.name << "\"\n";
      for( const auto& e : thread.events ) {
        z3::expr v = m.eval( e->guard );
        if( Z3_get_bool_value( v.ctx(), v)  != Z3_L_TRUE) {
          print_node( stream, e , "gray");
        }else
          print_node( stream, e );
        print_node( stream, thread.start_event );
        print_node( stream, thread.final_event );
        for( const auto& ep : e->prev_events )
          print_edge( stream, ep , e, "white" );
        for( const auto& ep : thread.final_event->prev_events )
          print_edge( stream, ep, thread.final_event, "white" );
        hb_enc::depends_set& dep_ses = e->data_dependency;
        for( auto iter = dep_ses.begin(); iter!=dep_ses.end(); ++iter ) {
          hb_enc::depends element = *iter;
          hb_enc::se_ptr val = element.e;
          z3::expr condition = element.cond;
          z3::expr v = m.eval( condition );
          if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE ) {
            print_edge( stream, val, e, "yellow");
          }else{
            print_edge( stream, val, e, "gray");
          }
        }
        hb_enc::depends_set& ctrl_ses = e->ctrl_dependency;
        for( auto iter = ctrl_ses.begin(); iter!=ctrl_ses.end(); ++iter ) {
          hb_enc::depends element = *iter;
          hb_enc::se_ptr val = element.e;
          z3::expr condition = element.cond;
          z3::expr v = m.eval( condition );
          if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE ) {
            print_edge( stream, val, e, "yellow");
          }else{
            print_edge( stream, val, e, "gray");
          }
        }
      }
      stream << " }\n";
      if( create_map.find( thread.name ) != create_map.end() ) {
        print_edge( stream , create_map.at(thread.name), thread.start_event, "brown" );
      }
      if( join_map.find( thread.name ) != join_map.end() ) {
        print_edge( stream, thread.final_event, join_map.at(thread.name).first, "brown" );
      }
    }
    if( is_mm_c11() ) {
      for( auto& it : reading_map ) {
        std::string bname = std::get<0>(it);
        z3::expr b = _z3.c.bool_const( bname.c_str() );
        z3::expr bv = m.eval( b );
        if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
          hb_enc::se_ptr wr = std::get<1>(it);
          hb_enc::se_ptr rd = std::get<2>(it);
          if( rd->is_rlx() || wr->is_rlx() )
            print_edge( stream, wr, rd, "blue,style=dashed" );
          else
            print_edge( stream, wr, rd, "blue" );
        }
      }
      for( auto& it : rel_seq_map ) {
        hb_enc::se_ptr rd = it.first;
        for( std::pair<std::string, hb_enc::se_ptr> b_wr : it.second ) {
          std::string bname = b_wr.first;
          z3::expr b = _z3.c.bool_const( bname.c_str() );
          z3::expr bv = m.eval( b );
          if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
            hb_enc::se_ptr wr = b_wr.second;
            print_edge( stream, wr, rd, "purple" );
          }
        }
      }
    }else{
      for( auto& it : reading_map ) {
        std::string bname = std::get<0>(it);
        z3::expr b = _z3.c.bool_const(  bname.c_str() );
        z3::expr bv = m.eval( b );
        if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
          hb_enc::se_ptr wr = std::get<1>(it);
          hb_enc::se_ptr rd = std::get<2>(it);
          if( rd->e_v->thread == wr->e_v->thread )
            print_edge( stream, wr, rd, "blue,style=dashed" );
          else print_edge( stream, wr, rd, "blue" );
          //fr
          // std::set< hb_enc::se_ptr > fr_wrs;
          auto it = wr_events.find(rd->prog_v);
          if( it != wr_events.end() ) { // fails for dummy variable
            for( const auto& after_wr : it->second ) {
              z3::expr v = m.eval( after_wr->guard );
              if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE) {
                if( _hb_encoding.eval_hb( m, wr, after_wr ) ) {
                  print_edge( stream, rd, after_wr, "orange,constraint=false" );
                }
              }
            }
          }
        }
      }
    }

    // // old inefficient sorting of write events
    // for(const cssa::variable& v1 : globals ) {
    //   std::set< hb_enc::se_ptr > wrs;
    //   auto it = wr_events.find( v1 );
    //   for( const auto& wr : it->second ) {
    //     z3::expr v = m.eval( wr->guard );
    //     if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE)
    //       wrs.insert( wr );
    //   }
    //   hb_enc::se_vec ord_wrs;
    //   while( !wrs.empty() ) {
    //     hb_enc::se_ptr min;
    //     for ( auto wr : wrs ) {
    //       if( min ) {
    //         if( _hb_encoding.eval_hb( m, wr, min ) ) {
    //           min = wr;
    //         }
    //       }else{
    //         min = wr;
    //       }
    //     }
    //     ord_wrs.push_back(min);
    //     wrs.erase( min );
    //   }

    //   for( unsigned i = 1; i < ord_wrs.size(); i++  ) {
    //     print_edge( stream, ord_wrs[i-1], ord_wrs[i], "green" );
    //   }
    // }


    for(const cssa::variable& v1 : globals ) {
      std::vector< std::pair< hb_enc::se_ptr, int > > wrs;
      auto it = wr_events.find( v1 );
      for( const auto& wr : it->second ) {
        z3::expr v = m.eval( wr->guard );
        if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE) {
          if( is_mm_c11() )
            v = m.eval( wr->get_c11_mo_solver_symbol() );
          else
            v = m.eval( wr->get_solver_symbol() );
          int val;
          if( Z3_get_numeral_int( v.ctx(), v, &val) ) {
            wrs.push_back( std::make_pair( wr, val ) );
          }else{
            program_error( "execution printing error!!" );
          }
        }
      }
      std::sort( wrs.begin(), wrs.end(), compare_events );

      for( unsigned i = 1; i < wrs.size(); i++  ) {
        print_edge( stream, wrs[i-1].first, wrs[i].first, "green" );
      }
    }

    if( is_mm_c11() ) {
      std::vector< std::pair< hb_enc::se_ptr, int > > es;
      for( const auto& e : all_sc) {
        z3::expr v = m.eval( e->guard );
        if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE) {
          v = m.eval( e->get_c11_sc_solver_symbol() );
          int val;
          if( Z3_get_numeral_int( v.ctx(), v, &val) ) {
            // std::cerr << e->get_c11_sc_solver_symbol() << " "<< val << "\n";
            es.push_back( std::make_pair( e, val ) );
          }else{
            program_error( "execution printing error!!" );
          }
        }
      }
      std::sort( es.begin(), es.end(), compare_events );

      for( unsigned i = 1; i < es.size(); i++  ) {
        print_edge( stream, es[i-1].first, es[i].first, "orange" );
      }
    }

    // for (hb_enc::hb hb : hbs) {
    //   stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << std::endl;
    // }
    stream << "}" << std::endl;
  }
}
