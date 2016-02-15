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


#ifndef TARA_INPUT_PROGRAM_H
#define TARA_INPUT_PROGRAM_H

#include <unordered_set>
#include <string>
#include <vector>
#include <memory>
#include <boost/concept_check.hpp>
#include "constants.h"
#include "input/instruction.h"
#include "helpers/z3interf.h"

namespace tara {
namespace input {
  
typedef std::pair<std::string,std::string> location_pair;

struct thread
{
  std::string name;
  variable_set locals;
  std::vector<std::shared_ptr<instruction>> instrs;
  
  unsigned size() const;
  std::shared_ptr<instruction>& operator [](unsigned i);
  const std::shared_ptr<instruction>& operator [](unsigned i) const;
  thread(std::string name);
};

  
class program
{
public:
  program();
  variable_set globals;
  std::vector<thread> threads;
  
  std::shared_ptr<instruction> precondition; 
  std::shared_ptr<instruction> postcondition; // wmm support
  std::shared_ptr<instruction> order_condition; 
  
  std::vector<location_pair> atomic_sections;
  std::vector<location_pair> happens_befores;
//----------------------------------------------------------------------------
// start of wmm support
//----------------------------------------------------------------------------
  mm_t mm = mm_t::none;
  bool is_wmm() const;
  bool is_mm_sc() const;
  bool is_mm_tso() const;
  bool is_mm_pso() const;
  bool is_mm_rmo() const;
  bool is_mm_power() const;
  void set_mm( mm_t );
  mm_t get_mm() const;
//----------------------------------------------------------------------------
// end of wmm support
//----------------------------------------------------------------------------

  bool is_global(const variable& name) const;
  bool is_global(const std::string& name) const;
  variable find_variable(int thread, const std::string& name) const;
  
  void convert_instructions(helpers::z3interf& z3);
  /**
   * @brief Converts the location names in the z3 instructions to z3 locations
   * 
   * @param z3 The z3 interface
   * @return void
   */
  void convert_names(helpers::z3interf& z3);
  
  /**
   * Throws an exception if an error was found
   */
  void check_correctness();
  inline std::shared_ptr<hb_enc::location> start_name() const {return _start_loc;}
  inline std::shared_ptr<hb_enc::location> end_name() const {return _end_loc;}
private:
  std::shared_ptr<hb_enc::location> _start_loc;
  std::shared_ptr<hb_enc::location> _end_loc;
  bool names_converted = false;
};
}}

#endif // TARA_INPUT_PROGRAM_H
