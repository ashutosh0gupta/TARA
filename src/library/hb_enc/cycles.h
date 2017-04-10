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


#ifndef TARA_HB_ENC_CYCLES_H
#define TARA_HB_ENC_CYCLES_H

// #include "cssa/program.h"
#include "api/options.h"
#include "hb_enc/symbolic_event.h"
#include "api/output/nf.h"
#include <boost/bimap.hpp>
#include <z3++.h>

namespace tara {
namespace hb_enc {


enum class edge_type {
  rf,  // c11 - rf edge
  fr,  // c11 - learned edge
  hb,  // hbs appear in the formula
  ppo, // preserved program order
  rpo  // relaxed program order
};

  bool is_weak_edge( edge_type );

class cycle_edge
{
public:
  cycle_edge( hb_enc::se_ptr _before, hb_enc::se_ptr _after, edge_type _type );
  hb_enc::se_ptr before;
  hb_enc::se_ptr after;
  edge_type type;
  friend bool ValueComp(const hb_enc::se_ptr &,const hb_enc::se_ptr &);
  
  bool operator==(const cycle_edge& b) const
  {
    return b.before == before && b.after == after && b.type == type;
  }
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
  bool add_edge( hb_enc::se_ptr _before, hb_enc::se_ptr _after, edge_type t );
  bool add_edge( hb_enc::se_ptr after, edge_type t );
  void close();
  void remove_prefix( hb_enc::se_ptr e );
  void remove_suffix( hb_enc::se_ptr e );
  void pop();
  void clear();
  unsigned has_cycle();
  unsigned size() { return edges.size(); };
  inline hb_enc::se_ptr first() {if( edges.size() > 0 )
      return edges[0].before;
    else
      return nullptr;
  };
  inline hb_enc::se_ptr last() {
    if( edges.size() > 0 )
      return edges.back().after;
    else
      return nullptr;
  };
  friend class fence_synth;
  friend std::ostream& operator<< (std::ostream& stream, const cycle& c);
  // std::unordered_map<unsigned,std::vector<hb_enc::se_ptr>>tid_to_se_ptr;
  std::vector<cycle_edge> edges;
private:
  std::string name;
  bool closed;
  std::vector<cycle_edge> relaxed_edges;

  void remove_suffix( unsigned i );
  void remove_prefix( unsigned i );

  bool has_relaxed( cycle_edge& edge) const;
public:
  bool is_dominated_by( cycle& c ) const;
};

typedef std::shared_ptr<cycle> cycle_ptr;


class cycles {

public:
  cycles( const helpers::z3interf& _z3) :
    z3(_z3) {};

  void find_cycles( const std::list<hb>& c, std::vector<cycle>& cycles );
  void find_cycles( const hb_enc::hb_vec& c, std::vector<cycle>& cycles );

  const tara::program* program;
  int verbose = 0;
private:
  typedef  std::vector< std::pair<hb_enc::se_ptr,hb_enc::se_ptr> > hb_conj;
  const helpers::z3interf& z3;
  std::ostream& out = std::cerr;// todo : redirected according to option

  // hb_conj hbs;
  std::vector<cycle_edge> hbs;
  std::vector<cycle_edge> learned_fr_edges;

  std::vector<hb_enc::se_vec> event_lists;

  std::map< hb_enc::se_ptr, int> index_map;
  std::map< hb_enc::se_ptr, int> lowlink_map;
  std::map< hb_enc::se_ptr, bool> on_stack;
  hb_enc::se_vec scc_stack;
  int scc_index;

  std::map< hb_enc::se_ptr, hb_enc::se_set > B_map;
  std::map< hb_enc::se_ptr, bool > blocked;
  cycle ancestor_stack;
  hb_enc::se_ptr root;


  // std::vector<hb_enc::se_vec>& event_lists;

  void succ( hb_enc::se_ptr e,
             const hb_enc::se_set& filter,
             std::vector< std::pair< hb_enc::se_ptr, edge_type> >& next_set );
  void find_sccs_rec( hb_enc::se_ptr e,
                      const hb_enc::se_set& filter,
                      std::vector< hb_enc::se_set >& sccs );

  void find_sccs( const hb_enc::se_set& filter,
                  std::vector< hb_enc::se_set >& sccs );

  void cycles_unblock( hb_enc::se_ptr e );

  void find_fr_edges( const hb_enc::hb_vec& c, hb_enc::se_set& all_es,
                      std::vector< cycle_edge >& new_edges );


  bool is_relaxed_dominated( const cycle& c , std::vector<cycle>& cs );
  bool find_true_cycles_rec( hb_enc::se_ptr e,
                             const hb_enc::se_set& scc,
                             std::vector<cycle>& found_cycles );

  void find_true_cycles( hb_enc::se_ptr e,
                         const hb_enc::se_set& scc,
                         std::vector<cycle>& found_cycles );
  // void find_cycles_internal( const hb_enc::se_set& all_events,
  //                            std::vector<cycle>& cycles);

  void insert_event( std::vector<hb_enc::se_vec>& event_lists, hb_enc::se_ptr e );

};

void debug_print( std::ostream& stream,
                  const std::vector<hb_enc::cycle_edge>& eds);

void debug_print( std::ostream& stream,
                  const hb_enc::cycle_edge& ed);

}

}


#endif // TARA_HB_ENC_CYCLES_H
