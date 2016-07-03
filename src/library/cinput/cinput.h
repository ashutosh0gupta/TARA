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

#ifndef TARA_CINPUT_PROGRAM_H
#define TARA_CINPUT_PROGRAM_H

#include "helpers/helpers.h"
#include "helpers/z3interf.h"
#include "api/options.h"
#include "hb_enc/symbolic_event.h"
#include "cinput/cinput_exception.h"

namespace tara {
namespace cinput {
  class loc{
  public:
    unsigned line;
    unsigned col;
    std::string file;
  };

  class thread {
  public:
    thread(helpers::z3interf& z3_, std::string name_):
      z3(z3_), name(name_) {}
    helpers::z3interf& z3;
    std::string name;
    hb_enc::se_vec events; // topologically sorted events
    void add_event( hb_enc::se_ptr e ) { events.push_back( e ); }
  };

  class program {
  public:
    program(helpers::z3interf& z3_): z3(z3_) { add_thread("caller"); }
    helpers::z3interf& z3;
    cssa::variable_set globals;
    z3::expr phi_ssa = z3.mk_true();
    std::map< unsigned, loc> inst_to_loc;
    // std::map< unsigned, std::string> id_to_thr_name;
    std::vector<thread> threads;
    hb_enc::name_to_ses_map se_store;
    unsigned thread_num = 0;
    unsigned add_thread( std::string str) {
      // id_to_thr_name[thread_num++] = str;
      thread thr( z3, str );
      threads.push_back( thr );
      thread_num++;
      return thread_num-1;
    };
    void append_ssa( z3::expr e) {
      phi_ssa = phi_ssa && e;
    }

    void add_event( unsigned i, hb_enc::se_ptr e ) {
      threads[i].add_event( e );
      se_store[e->name()] = e;
    }

    void add_global( std::string g, z3::sort sort ) {
      globals.insert( cssa::variable(g, sort) );
    }

    cssa::variable get_global( std::string gname ) {
      for( auto& g : globals ) {
        if( gname == g.name )
          return g;
      }
      cinput_error( "global variable " << gname << " not found!" );
      cssa::variable g(z3.c); // dummy code to suppress warning
      return g;
    }
  };

  program* parse_cpp_file( helpers::z3interf& z3_,
                           api::options& o_,
                           hb_enc::encoding& hb_encoding,
                           std::string& cfile );

  // class cinput_exception : public std::runtime_error{
  // public:
  //   input_exception(const char* what) : runtime_error(what) {};
  //   input_exception(const std::string& what) : runtime_error(what) {};
  // };

}}
#endif // TARA_CINPUT_PROGRAM_H
