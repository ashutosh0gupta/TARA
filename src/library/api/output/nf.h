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
 *
 */

#ifndef TARA_API_OUTPUT_NF_H
#define TARA_API_OUTPUT_NF_H

#include "api/output/output_base.h"

#include <z3++.h>
#include "hb_enc/encoding.h"
#include <vector>


namespace tara {
namespace api {
namespace output {

class nf : public output_base
{
public:
  typedef std::list< tara::hb_enc::hb > row_type;
  typedef std::list< row_type > result_type;

  /**
   * @brief Constructor
   * 
   * @param bad_dnf Produce a DNF of bad traces
   * @param bad_cnf Produce a CNF of bad traces
   * @param good_dnf Produce a DNF of good traces 
   * @param good_cnf Produce a CNF of good traces
   * @param verify Verify each output to see if it is correct
   * @param no_opt Do not try to optimise the DNF or CNF
   */
  
  nf(bool bad_dnf, bool bad_cnf, bool good_dnf, bool good_cnf, bool verify = false, bool no_opt = false); // sets the default for printing
  nf();
  const result_type& get_result(bool bad_not_good, bool dnf_not_cnf) const; // make sure the desired output is ready
  z3::expr get_result_expr(bool bad_not_good, bool dnf_not_cnf) const; // make sure the desired output is ready
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  void print(std::ostream& stream, bool machine_readable, bool bad, bool good, bool dnf, bool cnf, bool verify = false) const;
  //virtual z3::expr output() const;
  
  virtual void gather_statistics(metric& metric) const override;
  
  static void create_dnf(const z3::expr& formula, result_type& result, const tara::hb_enc::encoding& hb_encoding);
  static void print_one(std::ostream& stream, bool machine_readable, const result_type& result, bool dnf_not_cnf);
private:
  mutable result_type result_bad_dnf;
  mutable result_type result_bad_cnf;
  mutable result_type result_good_dnf;
  mutable result_type result_good_cnf;

  static void inverse_result(result_type& result);

  static void prune_implied_within(result_type& result, z3::solver& sol);
  static void prune_implied_outside(result_type& result, z3::solver& sol);
  /**
   * @brief Simply takes out hbs that are already implied by other hbs
   * 
   * @param result The hb to simplify
   * @param po The order of intra-thread instructions
   * @return void
   */
  static void prune_simple(result_type& result, const z3::expr& po);
  bool verify_result(std::ostream& stream, bool bad_not_good, z3::expr result) const;
  bool bad_dnf;
  bool bad_cnf;
  bool good_dnf;
  bool good_cnf;
  bool verify;
  bool no_opt;
  friend class bugs;
   
};
}
}
}

#endif // TARA_API_OUTPUT_NF_H
