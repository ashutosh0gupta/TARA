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

// enum class edge_type {
//   hb,  // hbs appear in the formula
//   ppo, // preserved program order
//   rpo  // relaxed program order
// };

// class cycle_edge
// {
// public:
//   cycle_edge( hb_enc::se_ptr _before, hb_enc::se_ptr _after, edge_type _type );
//   hb_enc::se_ptr before;
//   hb_enc::se_ptr after;
//   edge_type type;
//   friend bool ValueComp(const hb_enc::se_ptr &,const hb_enc::se_ptr &);
  
//   bool operator==(const cycle_edge& b)
//   {
//     return b.before == before && b.after == after && b.type == type;
//   }
// };

// class cycle
// {
// public:
//   cycle() {};
//   cycle(std::string _name) : name(_name) {};
//   cycle( cycle& _c, unsigned i );
//   // check if there is a shorter cycle
//   // if yes then .... ??
//   bool is_closed(){return closed;};
//   bool add_edge( hb_enc::se_ptr _before, hb_enc::se_ptr _after, edge_type t );
//   bool add_edge( hb_enc::se_ptr after, edge_type t );
//   void close();
//   void remove_prefix( hb_enc::se_ptr e );
//   void remove_suffix( hb_enc::se_ptr e );
//   void pop();
//   void clear();
//   unsigned has_cycle();
//   unsigned size() { return edges.size(); };
//   inline hb_enc::se_ptr first() {if( edges.size() > 0 )
//       return edges[0].before;
//     else
//       return nullptr;
//   };
//   inline hb_enc::se_ptr last() {
//     if( edges.size() > 0 )
//       return edges.back().after;
//     else
//       return nullptr;
//   };
//   friend class barrier_synthesis;
//   friend std::ostream& operator<< (std::ostream& stream, const cycle& c);
//   // std::unordered_map<unsigned,std::vector<hb_enc::se_ptr>>tid_to_se_ptr;
// private:
//   std::vector<cycle_edge> edges;
//   std::string name;
//   bool closed;
//   std::vector<cycle_edge> relaxed_edges;

//   void remove_suffix( unsigned i );
//   void remove_prefix( unsigned i );

//   bool has_relaxed( cycle_edge& edge);
//   bool is_dominated_by( cycle& c );
// };

// typedef std::shared_ptr<cycle> cycle_ptr;

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

  // std::map< hb_enc::se_ptr, int> index_map;
  // std::map< hb_enc::se_ptr, int> lowlink_map;
  // std::map< hb_enc::se_ptr, bool> on_stack;
  // hb_enc::se_vec scc_stack;
  // int scc_index;

  // std::map< hb_enc::se_ptr, hb_enc::se_set > B_map;
  // std::map< hb_enc::se_ptr, bool > blocked;
  // cycle ancestor_stack;
  // hb_enc::se_ptr root;

  // void succ( hb_enc::se_ptr e,
  //            const hb_conj& hbs,
  //            const std::vector<hb_enc::se_vec>& event_lists,
  //            const std::set<hb_enc::se_ptr>& filter,
  //            std::vector< std::pair< hb_enc::se_ptr, hb_enc::edge_type> >& next_set );
  // void find_sccs_rec( hb_enc::se_ptr e,
  //                     const hb_conj& hbs,
  //                     const std::vector<hb_enc::se_vec>& event_lists,
  //                     const std::set<hb_enc::se_ptr>& filter,
  //                     std::vector< std::set<hb_enc::se_ptr> >& sccs );

  // void find_sccs(  const hb_conj& hbs,
  //                  const std::vector<hb_enc::se_vec>& event_lists,
  //                  const std::set<hb_enc::se_ptr>& filter,
  //                  std::vector< std::set<hb_enc::se_ptr> >& sccs );

  // void cycles_unblock( hb_enc::se_ptr e );

  // bool is_relaxed_dominated( cycle& c , std::vector<cycle>& cs );
  // bool find_true_cycles_rec( hb_enc::se_ptr e,
  //                            const hb_conj& hbs,
  //                            const std::vector<hb_enc::se_vec>& event_lists,
  //                            const std::set<hb_enc::se_ptr>& scc,
  //                            std::vector<cycle>& found_cycles );

  // void find_true_cycles( hb_enc::se_ptr e,
  //                        const hb_conj& hbs,
  //                        const std::vector<hb_enc::se_vec>& event_lists,
  //                        const std::set<hb_enc::se_ptr>& scc,
  //                        std::vector<cycle>& found_cycles );
  // void find_cycles_internal( hb_conj& hbs,
  //                            std::vector<hb_enc::se_vec>& event_lists,
  //                            std::set<hb_enc::se_ptr>& all_events,
  //                            std::vector<cycle>& cycles);
  // void find_cycles( nf::row_type& c, std::vector<cycle>& cycles );
  // edge_type is_ppo(hb_enc::se_ptr before, hb_enc::se_ptr after);
  // void insert_event( std::vector<hb_enc::se_vec>& event_lists, hb_enc::se_ptr e );

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
  void assert_hard_constraints( z3::solver &s,std::vector<z3::expr>& cnstrs);
  void assert_soft_constraints( z3::solver&s ,z3::context& ctx,std::vector<z3::expr>& cnstrs, std::vector<z3::expr>& aux_vars );
  z3::expr at_most_one( z3::expr_vector& vars );
  int fu_malik_maxsat_step( z3::solver &s, z3::context &ctx, std::vector<z3::expr>& sofr_cnstrs, std::vector<z3::expr>& aux_vars );
  z3::model fu_malik_maxsat( z3::context& ctx, z3::expr hard_cnstrs, std::vector<z3::expr>& soft_cnstrs );

  long long unsigned time;
  std::string info;
};

}
}
}


#endif // TARA_API_OUTPUT_BARRIER_SYNTHESIS_H
