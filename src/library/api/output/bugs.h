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

#ifndef TARA_API_OUTPUT_BUGS_H
#define TARA_API_OUTPUT_BUGS_H

#include "api/output/nf.h"
#include "helpers/helpers.h"

namespace tara {
namespace api {
namespace output {
  
enum class bug_type {
  two_stage,
  data_race,
  define_use,
  atomicity_violation,
};

struct bug {
  bug(bug_type type);
  bug(bug_type type, std::list<hb_enc::tstamp_ptr> locations);
  bug_type type;
  std::list<hb_enc::tstamp_ptr> locations;
  friend std::ostream& operator<< (std::ostream& stream, const bug& bug);
  bool operator==(const bug& rhs) const;
  bool operator!=(const bug& rhs) const;
};
  
class bugs : public output_base
{
public:
  bugs( options& o_, helpers::z3interf& _z3 );
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  virtual void init(const hb_enc::encoding& hb_encoding, const z3::solver& sol_desired, const z3::solver& sol_undesired, std::shared_ptr<const tara::program> program) override;
  virtual void output(const z3::expr& output) override;
  virtual void gather_statistics(metric& metric) const override;
private:
  bool print_nfs;
  nf normal_form;
  std::list<bug> bug_list;
  nf::result_type unclassified;
// patterns we recognise
  void find_bugs(nf::result_type& dnf);
  /**
   * @brief Remove from a row any read to an assume
   * 
   * @param conj ...
   * @return bool
   */
  void clean_row(tara::api::output::nf::row_type& conj);
  bool define_use(tara::api::output::nf::row_type& conj);
  bool race_condition(tara::api::output::nf::row_type& conj);
  bool two_stage(tara::api::output::nf::row_type& conj, std::list< tara::api::output::nf::row_type >::iterator next, tara::api::output::nf::result_type& list);
  
  /**
   * @brief Determines if there is a trace that allows for all assignments to variable be after loc
   * 
   * @param variable ...
   * @param loc ...
   * @param hbs ...
   * @return bool
   */
  bool first_assignment(const cssa::variable& variable, hb_enc::tstamp_ptr loc, const nf::row_type& hbs);
  
  long long unsigned time;
};
}
}
}

#endif // TARA_API_BUGS_SYNTHESIS_H
