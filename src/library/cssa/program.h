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

#include "program/program.h"
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

class program : public tara::program {
public:
  program(helpers::z3interf& z3, hb_enc::encoding& hb_encoding, const input::program& input);
  program(const program&) = delete;
  program& operator=(const program&) = delete;
  std::vector< std::vector < bool > > build_po() const;
private:
  
  struct pi_needed {
    variable name;
    variable orig_name;
    std::shared_ptr<instruction> last_local; // can be NULL if none
    unsigned thread;
    hb_enc::location_ptr loc;

    pi_needed( variable name,
               variable orig_name,
               std::shared_ptr<instruction> last_local,
               unsigned thread, hb_enc::location_ptr loc)
      : name(name)
      , orig_name(orig_name)
      , last_local(last_local)
      , thread(thread)
      , loc(loc) {}
  };

  void build_threads(const input::program& input);
  void build_hb(const input::program& input);
  void build_pre(const input::program& input);
  void build_pis(std::vector<pi_needed>& pis, const input::program& input);

  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------

public:
  std::unordered_multimap <int,int>tid_to_instr;
private:
  void wmm_build_cssa_thread( const input::program& input );
  void wmm_build_ssa( const input::program& input );
  void wmm_build_pre( const input::program& input );
  void wmm_build_post( const input::program& input,
                       std::unordered_map<std::string, std::string>& thread_vars );
  void wmm(const input::program& input);

  void wmm_print_dot( std::ostream& stream, z3::model m ) const;
  void wmm_print_dot( z3::model m ) const;


public:
  bool has_barrier_in_range( unsigned tid, unsigned start_inst_num,
                             unsigned end_inst_num ) const;

  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------

public:
  // set of instructions that are assertions
  std::unordered_set<std::shared_ptr<instruction>> assertion_instructions;
  // maps a pi variable to a set of function parts
  std::unordered_map<std::string, std::vector<pi_function_part>> pi_functions;
  // where a local variable is written
  std::unordered_map<std::string, std::shared_ptr<instruction>> variable_written;
  //std::unordered_map<std::shared_ptr<cssa::instruction>,std::shared_ptr<cssa::variable>> instr_to_var; // where a variable is read/written
  
  //tara::input::variable search_for_variable_in_read(tara::input::variable,std::shared_ptr<cssa::thread>, int); 

  /**
   * @brief Set of initial variables (used to get input values)
   * 
   */
  variable_set initial_variables;
  
  const instruction& lookup_location(const tara::hb_enc::location_ptr& location) const;
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
