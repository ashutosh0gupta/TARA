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

#ifndef TARA_API_OUTPUT_BARRIER_SYNTHESIS_H
#define TARA_API_OUTPUT_BARRIER_SYNTHESIS_H

#include "cssa/program.h"
#include "hb_enc/symbolic_event.h"
#include "hb_enc/cycles.h"
#include "api/output/nf.h"
#include <boost/bimap.hpp>
#include <z3++.h>
using namespace tara::cssa;

namespace tara {
namespace api {
namespace output {

  typedef  std::vector< std::pair<hb_enc::se_ptr,hb_enc::se_ptr> > hb_conj;
  typedef hb_enc::cycle cycle;
  typedef hb_enc::edge_type edge_type;

class barrier_synthesis : public output_base
{
public:
  barrier_synthesis( helpers::z3interf& _z3, bool verify, bool verbose );
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  virtual void init( const hb_enc::encoding& hb_encoding,
                     const z3::solver& sol_desired,
                     const z3::solver& sol_undesired,
                     std::shared_ptr<const tara::program> program ) override;
  virtual void output(const z3::expr& output) override;
  virtual void gather_statistics(metric& metric) const override;

  bool enable_lw_fences = false;

  // friend class cycle;
private:
  bool verbose;
  nf normal_form;

  // std::shared_ptr<const cssa::program> program;
  std::vector< std::vector<cycle> > all_cycles;
  // z3::expr maxsat_hard;
  // std::vector<z3::expr> maxsat_soft;

  //-------------------------------------------------------------
  // new cycle detection

  hb_enc::cycles cycle_finder;

  void find_cycles(nf::result_type& bad_dnf);

  //-------------------------------------------------------------

  std::vector<z3::expr> cut;
  std::vector<z3::expr> soft;
  boost::bimap<z3::expr, cycle* >z3_to_cycle;
  std::unordered_map< unsigned, std::vector<hb_enc::se_ptr> > tid_to_se_ptr;
  std::unordered_map<hb_enc::se_ptr,z3::expr> segment_map;
  // std::map<std::string, hb_enc::se_ptr> rev_segment_map;

  std::map< std::pair<hb_enc::se_ptr,hb_enc::se_ptr>, z3::expr > wr_ord_map;
  std::map< std::tuple<hb_enc::se_ptr,hb_enc::se_ptr,hb_enc::se_ptr>, z3::expr >
  hist_map;
  std::map< hb_enc::se_ptr, z3::expr > barrier_map;
  std::map< hb_enc::se_ptr, z3::expr > light_barrier_map;

  z3::expr get_h_var_bit(hb_enc::se_ptr b, hb_enc::se_ptr i, hb_enc::se_ptr j);
  z3::expr get_write_order_bit( hb_enc::se_ptr b, hb_enc::se_ptr k );
  z3::expr get_barr_bit( hb_enc::se_ptr k );
  z3::expr get_lw_barr_bit( hb_enc::se_ptr k );

  z3::expr mk_edge_constraint( hb_enc::se_ptr& before,
                               hb_enc::se_ptr& after, z3::expr& e );
  void gen_max_sat_problem();
  void gen_max_sat_problem_new();
  void print_all_cycles( std::ostream& stream ) const;

  std::vector<hb_enc::se_ptr>barrier_where;
  std::vector<hb_enc::se_ptr>soft_barrier_where;

  z3::expr get_fresh_bool();
  // void assert_hard_constraints( z3::solver &s,std::vector<z3::expr>& cnstrs);
  void assert_soft_constraints( z3::solver&s , std::vector<z3::expr>& cnstrs, std::vector<z3::expr>& aux_vars );
  z3::expr at_most_one( z3::expr_vector& vars );
  int fu_malik_maxsat_step( z3::solver &s, std::vector<z3::expr>& sofr_cnstrs, std::vector<z3::expr>& aux_vars );
  z3::model fu_malik_maxsat( z3::expr hard_cnstrs, std::vector<z3::expr>& soft_cnstrs );

  long long unsigned time;
  std::string info;
};

}
}
}


#endif // TARA_API_OUTPUT_BARRIER_SYNTHESIS_H
