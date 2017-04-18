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

#include "remove_non_cycled.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_non_cycled::remove_non_cycled(const z3interf& z3, const tara::program& program) : prune_base(z3, program), cycle_finder( z3 )
{
  cycle_finder.program = &program;
  cycle_finder.verbose = program.options().print_pruning;
  if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso()
      || program.is_mm_rmo() || program.is_mm_alpha() || program.is_mm_c11() ){
  }else{
    prune_error("remove_non_cycled does not support memory model!");
  }
}

string remove_non_cycled::name()
{
  return string("remove_non_cycled");
}

inline bool match_edge_hb_type( hb_enc::hb_ptr hb, hb_enc::cycle_edge edge) {
  if( edge.type == hb_enc::edge_type::hb )
    return hb->is_hb()||hb->is_partial_ord_hb();
  if( edge.type == hb_enc::edge_type::rf ) return hb->is_rf();
  if( edge.type == hb_enc::edge_type::fr )
    return (hb->is_hb()||hb->is_partial_ord_hb()) && hb->is_neg;
  return false;
}

hb_enc::hb_vec remove_non_cycled::prune( const hb_enc::hb_vec& hbs,
                                         const z3::model& m ) {
  assert( program.is_mm_declared() );
  std::vector<hb_enc::cycle> found_cycles;
  cycle_finder.find_cycles( hbs, found_cycles);
  assert( found_cycles.size() > 0 );
  hb_enc::hb_vec temp = hbs;
  hb_enc::hb_vec result;
  for( auto& c : found_cycles ) {
    // std::cout << c << "\n";
    for( auto& edge : c.edges ) {
      if( edge.type == hb_enc::edge_type::hb ||
          edge.type == hb_enc::edge_type::rf ||
          edge.type == hb_enc::edge_type::fr ) {
        bool found = false;
        for( auto it = temp.begin(); it != temp.end();it++ ) {
          auto& h = *it;
          if( h->e1 == edge.before && h->e2 == edge.after
              && match_edge_hb_type( h, edge) ) {
            result.push_back( h );
            temp.erase( it );
            found = true;
            break;
          }
        }
        if( !found ) {
          // assert( edge.type == hb_enc::edge_type::fr );
          auto cause_hbs = cycle_finder.fr_cause( edge );
          for( auto& hb : cause_hbs ) {
            auto it = std::find( temp.begin(), temp.end(), hb );
            if( it != temp.end() ) {
              result.push_back(*it);
              temp.erase( it );
            }
          }
        }
      }
    }
  }
  return result;
}


