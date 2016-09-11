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
  if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso() || program.is_mm_rmo() || program.is_mm_alpha()){
  }else{
    throw std::runtime_error("remove_non_cycled does not support memory model!");
  }
}

string remove_non_cycled::name()
{
  return string("remove_non_cycled");
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
      if( edge.type == hb_enc::edge_type::hb ) {
        for( auto it = temp.begin(); it != temp.end();it++ ) {
          auto& h = *it;
          if( h->e1 == edge.before && h->e2 == edge.after ) {
            result.push_back( h );
            temp.erase( it );
            break;
          }
        }
      }
    }
  }
  return result;
}


