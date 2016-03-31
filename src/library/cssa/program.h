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


#ifndef TARA_CSSA_PROGRAM_H
#define TARA_CSSA_PROGRAM_H

#include "input/program.h"
#include "helpers/z3interf.h"
#include <vector>
#include <list>
#include "cssa/thread.h"
#include <unordered_map>
#include <unordered_set>
#include <boost/concept_check.hpp>
#include<utility>

namespace tara{
namespace cssa {
  
struct pi_function_part {
  variable_set variables;
  z3::expr hb_exression;
  pi_function_part(variable_set variables, z3::expr hb_exression);
  pi_function_part(z3::expr hb_exression);
};

class program
{
public:
  program(helpers::z3interf& z3, hb_enc::encoding& hb_encoding, const input::program& input);
  program(const program&) = delete;
  program& operator=(const program&) = delete;
  std::vector< std::vector < bool > > build_po() const;
  //std::unordered_map<std::string,int>instr_to_MatIndex;
  unsigned no_of_threads() const;
  unsigned no_of_instructions(unsigned tid) const;
  unsigned total_instructions() const;
  std::string instr_name(unsigned tid,unsigned instr_no) const;
private:
  std::vector<std::shared_ptr<cssa::thread>> threads;
  helpers::z3interf& _z3;
  hb_enc::encoding& _hb_encoding;
  
  struct pi_needed {
    variable name;
    variable orig_name;
    std::shared_ptr<instruction> last_local; // can be NULL if none
    unsigned thread;
    hb_enc::location_ptr loc;
    
    pi_needed(variable name, variable orig_name, std::shared_ptr<instruction> last_local, unsigned thread, hb_enc::location_ptr loc) : name(name), orig_name(orig_name), last_local(last_local), thread(thread), loc(loc) {}
  };
  
  void build_threads(const input::program& input);
  void build_hb(const input::program& input);
  void build_pre(const input::program& input);
  void build_pis(std::vector<pi_needed>& pis, const input::program& input);

  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------

public:
  name_to_ses_map se_store;
  se_to_ses_map dependency_relation;
  se_set init_loc;
  se_set post_loc;
  var_to_ses_map wr_events;
  var_to_se_vec_map rd_events;
  std::unordered_multimap <int,int>tid_to_instr;
  // class isEq{
  // public:
  //   inline bool operator( )( const z3::expr& a, const z3::expr&b ) {
  //     // return z3::eq(a,b);
  //     const Z3_ast ap = a;
  //     const Z3_ast bp = b;
  //     return ap == bp;
  //   }
  // };
  // std::map<z3::expr, std::pair<se_ptr,se_ptr>, isEq > reading_map;
  std::set< std::tuple<std::string,se_ptr,se_ptr> > reading_map;
private:
  void wmm_build_cssa_thread( const input::program& input );
  void wmm_build_ssa( const input::program& input );
  void wmm_mk_distinct_events();
  void wmm_build_threads( const input::program& input );
  void wmm_build_hb( const input::program& input ); // should be removed
  void wmm_build_pre( const input::program& input );
  void wmm_build_post( const input::program& input, std::unordered_map<std::string, std::string>& thread_vars );
  void wmm_build_pis(std::vector<pi_needed>& pis, const input::program& input);
  void wmm(const input::program& input);
  void wmm_build_ses();
  void wmm_build_barrier();
  void wmm_add_barrier(int tid, int instr);

  z3::expr wmm_insert_tso_barrier( thread & thread, unsigned instr, se_ptr );
  z3::expr wmm_insert_pso_barrier( thread & thread, unsigned instr, se_ptr );
  z3::expr wmm_insert_rmo_barrier( thread & thread, unsigned instr, se_ptr );
  z3::expr wmm_insert_barrier( unsigned tid, unsigned instr );

  z3::expr wmm_mk_hb ( const cssa::se_ptr& before, const cssa::se_ptr& after );
  z3::expr wmm_mk_hbs( const cssa::se_ptr& before, const cssa::se_ptr& after );
  z3::expr wmm_mk_hbs( const cssa::se_set& before, const cssa::se_set& after );
  z3::expr wmm_mk_hbs( const cssa::se_set& before, const cssa::se_ptr& after );
  z3::expr wmm_mk_hbs( const cssa::se_ptr& before, const cssa::se_set& after );
  z3::expr wmm_mk_ghb( const cssa::se_ptr& before, const cssa::se_ptr& after );

  bool hb_eval( const z3::model& model,
                const cssa::se_ptr& before, const cssa::se_ptr& after ) const;

  z3::expr get_rf_bvar( const variable& v1, se_ptr wr, se_ptr rd,
                        bool record=true );

  void wmm_build_sc_ppo   ( thread& thread );
  void wmm_build_tso_ppo  ( thread& thread );
  void wmm_build_pso_ppo  ( thread& thread );
  void wmm_build_rmo_ppo  ( thread& thread );
  void wmm_build_alpha_ppo( thread& thread );
  void wmm_build_power_ppo( thread& thread );
  void wmm_build_ppo();

  void wmm_print_dot( std::ostream& stream, z3::model m ) const;
  void wmm_print_dot( z3::model m ) const;
  void wmm_test_ppo(); // TODO: remove when confident

  bool in_grf( const cssa::se_ptr& wr, const cssa::se_ptr& rd );
  bool anti_ppo_read( const cssa::se_ptr& wr, const cssa::se_ptr& rd );
  bool anti_po_loc_fr( const cssa::se_ptr& rd, const cssa::se_ptr& wr );

private:
  mm_t mm = mm_t::none;

public:
  bool is_mm_declared() const;
  bool is_wmm() const;
  bool is_mm_sc() const;
  bool is_mm_tso() const;
  bool is_mm_pso() const;
  bool is_mm_rmo() const;
  bool is_mm_alpha() const;
  bool is_mm_power() const;
  void set_mm( mm_t );
  mm_t get_mm() const;
  void unsupported_mm() const;
  bool has_barrier_in_range( unsigned tid, unsigned start_inst_num,
                             unsigned end_inst_num ) const;
  z3::expr wf      = _z3.c.bool_val(true);
  z3::expr rf      = _z3.c.bool_val(true);
  z3::expr grf     = _z3.c.bool_val(true);
  z3::expr ws      = _z3.c.bool_val(true);

  z3::expr fr      = _z3.c.bool_val(true);
  z3::expr rf_some = _z3.c.bool_val(true);
  z3::expr rf_ws   = _z3.c.bool_val(true);
  z3::expr rf_grf  = _z3.c.bool_val(true);
  z3::expr rf_val  = _z3.c.bool_val(true);
  z3::expr phi_post = _z3.c.bool_val(true);

  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------

public:
  z3::expr phi_pre = _z3.c.bool_val(true);
  z3::expr phi_po = _z3.c.bool_val(true);
  z3::expr phi_vd = _z3.c.bool_val(true);
  z3::expr phi_pi = _z3.c.bool_val(true);
  z3::expr phi_prp = _z3.c.bool_val(true);
  z3::expr phi_fea = _z3.c.bool_val(true); // feasable traces
  z3::expr phi_distinct = _z3.c.bool_val(true); // ensures that all locations are distinct
  //z3::expr fences = _z3.c.bool_val(true);
  std::unordered_set<std::shared_ptr<instruction>> assertion_instructions; // set of instructions that are assertions
  std::unordered_map<std::string, std::vector<pi_function_part>> pi_functions; // maps a pi variable to a set of function parts
  std::unordered_map<std::string, std::shared_ptr<instruction>> variable_written; // where a local variable is written
  //std::unordered_map<std::shared_ptr<cssa::instruction>,std::shared_ptr<cssa::variable>> instr_to_var; // where a variable is read/written
  
//tara::input::variable search_for_variable_in_read(tara::input::variable,std::shared_ptr<cssa::thread>, int); 

  /**
   * @brief Set of initial variables (used to get input values)
   * 
   */
  variable_set initial_variables;
  
  const thread& operator[](unsigned i) const;
  unsigned size() const;
  const instruction& lookup_location(const tara::hb_enc::location_ptr& location) const;
  variable_set globals;
    // variable_set new_globals;
  inline const hb_enc::encoding& hb_encoding() const {return _hb_encoding; }
  inline const helpers::z3interf& z3() const { return _z3; }
public: /* functions */
  /**
   * @brief Gets the initial values of global variables
   */
  z3::expr get_initial(const z3::model& m) const;
  void print_hb(const z3::model& m, std::ostream& out, bool machine_readable = false) const;
  std::list<z3::expr> get_hbs(z3::model& m) const;
  void print_dot(std::ostream& stream, std::vector<hb_enc::hb>& hbs) const;
  
  bool is_global(const variable& name) const;
  std::vector< std::shared_ptr<const instruction> > get_assignments_to_variable(const tara::cssa::variable& variable) const;
};
}}

#endif // TARA_CSSA_PROGRAM_H
