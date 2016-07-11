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

#ifndef TARA_PROGRAM_H
#define TARA_PROGRAM_H

#include "helpers/helpers.h"
#include "helpers/z3interf.h"
#include "api/options.h"
#include "hb_enc/symbolic_event.h"
// #include "cinput/cinput_exception.h"

namespace tara {
  namespace cssa{
    class program;
  }
  class thread;
  class loc{
  public:
    unsigned line;
    unsigned col;
    std::string file;
  };

  class instruction {
  public:
    hb_enc::location_ptr loc;
    z3::expr instr;
    z3::expr path_constraint;
    thread* in_thread;
    std::string name;
    // variable_set variables_read_copy;
    cssa::variable_set variables_read; // the variable names with the hash
    cssa::variable_set variables_write; // the variable names with the hash
    cssa::variable_set variables_read_orig; // the original variable names
    cssa::variable_set variables_write_orig; // the original variable names, unprimed
    cssa::variable_set variables() const;
    cssa::variable_set variables_orig() const;
    instruction_type type;
    cssa::variable_set havok_vars;

    //--------------------------------------------------------------------------
    // WMM support
    //--------------------------------------------------------------------------

    hb_enc::se_set rds;
    hb_enc::se_set wrs;
    hb_enc::se_set barr; //the event created for barrier if the instruction is barrier type
  public:
    //--------------------------------------------------------------------------
    // End WMM support
    //--------------------------------------------------------------------------
  
    instruction(helpers::z3interf& z3, hb_enc::location_ptr location, thread* thread, std::string& name, instruction_type type, z3::expr original_expression);
    friend std::ostream& operator<< (std::ostream& stream, const instruction& i);
    void debug_print( std::ostream& o );
  private:
    z3::expr original_expr;
  };

  class thread {
  public:
    thread(helpers::z3interf& z3_, std::string& name_):
      z3(z3_), name(name_) {}
    hb_enc::se_ptr start_event, final_event;
    helpers::z3interf& z3;
    const std::string name;
    hb_enc::se_vec events; // topologically sorted events
    z3::expr phi_ssa = z3.mk_true();
    z3::expr phi_prp = z3.mk_false();
    void add_event( hb_enc::se_ptr e ) { events.push_back( e ); }
    void set_start_event( hb_enc::se_ptr e ) { start_event = e; }
    void set_final_event( hb_enc::se_ptr e ) { final_event = e; }
    void append_ssa( z3::expr e) {
      phi_ssa = phi_ssa && e;
    }

    void append_property( z3::expr prp) {
      phi_prp = phi_prp || prp;
    }

    //old thread

    thread( helpers::z3interf& z3_,
            const std::string& name, cssa::variable_set locals );
    thread(thread& ) = delete;
    thread& operator=(thread&) = delete;

    std::vector<std::shared_ptr<instruction>> instructions;
    // const std::string name;
    std::unordered_map<std::string,std::vector<std::shared_ptr<instruction>>> global_var_assign;
    cssa::variable_set locals;

    bool operator==(const thread &other) const;
    bool operator!=(const thread &other) const;

    unsigned size() const;
    instruction& operator [](unsigned i);
    const instruction& operator [](unsigned i) const;
    void add_instruction(const std::shared_ptr< tara::instruction >& instr);
  };

  class program {
  private:
    unsigned thread_num = 0;
    std::vector<std::shared_ptr<thread>> threads;
    mm_t mm = mm_t::none;
    helpers::z3interf& _z3;
    hb_enc::encoding& _hb_encoding;

  public:
    program( helpers::z3interf& z3_,
             hb_enc::encoding& hb_encoding_
             ): _z3(z3_), _hb_encoding(hb_encoding_) {}

    cssa::variable_set globals;
    cssa::variable_set allocated; // temp allocations
    std::map< unsigned, loc> inst_to_loc;
    std::map< std::string, hb_enc::se_ptr> create_map, join_map;
    hb_enc::name_to_ses_map se_store;

    inline const hb_enc::encoding& hb_encoding() const {return _hb_encoding; }
    inline const helpers::z3interf& z3() const { return _z3; }

    //--------------------------------------------------------------------------
    // cssa::program variables moved here
    //--------------------------------------------------------------------------
    z3::expr phi_ses  = _z3.mk_true();
    z3::expr phi_post = _z3.mk_true();
    z3::expr phi_pre  = _z3.mk_true();//z3.c.bool_val(true);
    z3::expr phi_po   = _z3.mk_true();
    z3::expr phi_vd   = _z3.mk_true();
    z3::expr phi_pi   = _z3.mk_true();
    z3::expr phi_prp  = _z3.mk_true();
    z3::expr phi_fea  = _z3.mk_true(); // feasable traces
    z3::expr phi_distinct = _z3.mk_true(); //ensures that all locations are distinct

    inline bool is_mm_declared() const;
    inline bool is_wmm() const;
    inline bool is_mm_sc() const;
    inline bool is_mm_tso() const;
    inline bool is_mm_pso() const;
    inline bool is_mm_rmo() const;
    inline bool is_mm_alpha() const;
    inline bool is_mm_power() const;
    void set_mm( mm_t );
    mm_t get_mm() const;
    void unsupported_mm() const;

    hb_enc::se_to_ses_map data_dependency;
    hb_enc::se_to_ses_map ctrl_dependency;
    hb_enc::se_set init_loc;
    hb_enc::se_set post_loc;
    hb_enc::var_to_ses_map wr_events;
    hb_enc::var_to_se_vec_map rd_events;
    std::set< std::tuple<std::string,hb_enc::se_ptr,hb_enc::se_ptr> > reading_map;

    const tara::thread& operator[](unsigned i) const;
    unsigned size() const;

    inline unsigned no_of_threads() const {
      return threads.size();
    }

    inline const thread& get_thread(unsigned t) const {
      assert( t < threads.size() );
      return *threads[t];
    }

    unsigned add_thread( std::string str) {
      auto thr = std::make_shared<thread>( _z3, str );
      threads.push_back( thr );
      thread_num++;
      return thread_num-1;
    }

    void append_ssa( unsigned thread_id, z3::expr e) {
      threads[thread_id]->append_ssa( e );
    }

    void append_property( unsigned thread_id, z3::expr prp) {
      threads[thread_id]->append_property( prp );
    }


    void add_event( unsigned i, hb_enc::se_ptr e ) {
      threads[i]->add_event( e );
      se_store[e->name()] = e;
    }

    void add_event( unsigned i, hb_enc::se_set es ) {
      for( auto e : es ) {
        add_event( i, e );
      }
    }

    void set_start_event( unsigned i, hb_enc::se_ptr e ) {
      threads[i]->set_start_event( e );
    }

    void set_final_event( unsigned i, hb_enc::se_ptr e ) {
      threads[i]->set_final_event( e );
    }

    void add_create( unsigned thr_id, hb_enc::se_ptr e, std::string fname ) {
      add_event( thr_id, e );
      if( create_map.find( fname) != create_map.end() )
        program_error( "function launch multiple times not supported!" );
      create_map[ fname ] = e;
    }

    void add_join( unsigned thr_id, hb_enc::se_ptr e, std::string fname ) {
      add_event( thr_id, e );
      if( join_map.find( fname) != create_map.end() )
        program_error( "function launch multiple times not supported!" );
      join_map[ fname ] = e;
    }

    void add_global( std::string g, z3::sort sort ) {
      globals.insert( cssa::variable(g, sort) );
    }

    cssa::variable get_global( std::string gname ) {
      for( auto& g : globals ) {
        if( gname == g.name )
          return g;
      }
      program_error( "global variable " << gname << " not found!" );
      cssa::variable g(_z3.c); // dummy code to suppress warning
      return g;
    }

    void add_allocated( std::string g, z3::sort sort ) {
      allocated.insert( cssa::variable(g, sort) );
    }

    cssa::variable get_allocated( std::string gname ) {
      for( auto& g : allocated ) {
        if( gname == g.name )
          return g;
      }
      program_error( "allocated name " << gname << " not found!" );
      cssa::variable g(_z3.c); // dummy code to suppress warning
      return g;
    }

    friend cssa::program;
  };

inline bool program::is_mm_declared() const {  return mm != mm_t::none; }
inline bool program::is_wmm()         const {  return mm != mm_t::sc;   }
inline bool program::is_mm_sc()       const {  return mm == mm_t::sc;   }
inline bool program::is_mm_tso()      const {  return mm == mm_t::tso;  }
inline bool program::is_mm_pso()      const {  return mm == mm_t::pso;  }
inline bool program::is_mm_rmo()      const {  return mm == mm_t::rmo;  }
inline bool program::is_mm_alpha()    const {  return mm == mm_t::alpha;}
inline bool program::is_mm_power()    const {  return mm == mm_t::power;}

inline mm_t program::get_mm()         const { return mm; }
inline void program::set_mm(mm_t _mm)       { mm = _mm;  }

inline void program::unsupported_mm() const {
  std::string msg = "unsupported memory model: " + string_of_mm( mm ) + "!!";
  program_error( msg );
}


}
#endif // TARA_PROGRAM_H
