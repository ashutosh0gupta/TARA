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
#include "cssa/wmm.h"
#include "api/output/nf.h"

using namespace tara::cssa;

namespace tara {
namespace api {
namespace output {

enum class edge_type {
  hb,  // hbs appear in the formula
  ppo, // preserved program order
  rpo  // relaxed program order
};

class cycle_edge
{
public:
  cycle_edge( se_ptr _before, se_ptr _after, edge_type _type );
  se_ptr before;
  se_ptr after;
  edge_type type;
};

class cycle
{
public:
  cycle() {};
  cycle(std::string _name) : name(_name) {};
  cycle( cycle& _c, unsigned i );
  // check if there is a shorter cycle
  // if yes then .... ??
  bool is_closed(){return closed;};
  bool add_edge( se_ptr _before, se_ptr _after, edge_type t );
  bool add_edge( se_ptr after, edge_type t );
  void close();
  void remove_prefix( se_ptr e );
  void remove_suffix( se_ptr e );
  void pop();
  unsigned has_cycle();
  unsigned size() { return edges.size(); };
  inline se_ptr first() { assert( edges.size() > 0 ); return edges[0].before; };
  inline se_ptr last() { assert( edges.size() > 0 ); return edges.back().before; };

  friend std::ostream& operator<< (std::ostream& stream, const cycle& c);
private:
  std::vector<cycle_edge> edges;
  std::string name;
  bool closed;

  void remove_suffix( unsigned i );
  void remove_prefix( unsigned i );

};

class barrier_synthesis : public output_base
{
public:
  barrier_synthesis(bool verify, bool print_nfs);
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  virtual void init( const hb_enc::encoding& hb_encoding,
                     const z3::solver& sol_desired,
                     const z3::solver& sol_undesired,
                     std::shared_ptr<const cssa::program> program ) override;
  virtual void output(const z3::expr& output) override;
  virtual void gather_statistics(metric& metric) const override;
private:
  bool print_nfs;
  nf normal_form;

  std::shared_ptr<const cssa::program> program;
  std::vector< std::vector<cycle> > all_cycles;
  // z3::expr maxsat_hard;
  // std::vector<z3::expr> maxsat_soft;

  void find_cycles(nf::result_type& cnf);
  edge_type is_ppo(se_ptr before, se_ptr after);
  void insert_event( std::vector<se_vec>& event_lists, se_ptr e );

  void gen_max_sat_problem();

//   std::list<hb_enc::hb> wait_notifies;
//   std::list<lock> locks;
//   std::list<barrier> barriers;
//   void synthesise(nf::result_type& disjunct);
//   bool find_lock(const tara::api::output::nf::row_type& disjunct);
//   bool find_barrier(const nf::row_type& disjunct, nf::result_type::iterator current, nf::result_type& list);
//   // /**
//   //  * @brief Merge the locks accross threads.
//   //  */
//   // void merge_locks_multithread();
//   // /**
//   //  * @brief Merge the locks within a thread.
//   //  */
//   // void merge_locks();
//   // void merge_barriers();
//   // void merge_barriers_multithread();

  long long unsigned time;
  std::string info;
};

}
}
}


#endif // TARA_API_OUTPUT_BARRIER_SYNTHESIS_H
