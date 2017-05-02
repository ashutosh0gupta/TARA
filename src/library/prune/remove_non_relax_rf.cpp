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

#include "remove_non_relax_rf.h"
#include "hb_enc/hb.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_non_relax_rf::remove_non_relax_rf( const z3interf& z3, const tara::program& program) : prune_base(z3, program)
{
  if( program.is_mm_c11() ){
  }else{
    throw std::runtime_error("remove_non_relax_rf does not support current memory model!");
  }

  //check if each variable has at least on relax write
  for( auto& g : program.globals ) {
    for( auto& wr : program.wr_events.at(g) ) {
      if( wr->is_rlx() ) {
        relaxed_wrs.insert( g );
        break;
      }
    }
    // Should rd be included?
    // for( auto& rd : program.rd_events.at(g) ) {
    //   if( rd->is_rlx() ) {
    //     relaxed_wrs.insert( g );
    //     break;
    //   }
    // }
  }
}

string remove_non_relax_rf::name()
{
  return string("remove_non_relax_rf");
}

hb_enc::hb_vec remove_non_relax_rf::prune( const hb_enc::hb_vec& hbs,
                                           const z3::model& m ) {
  assert( program.is_mm_declared() );
  hb_enc::hb_vec result = hbs;
  for (auto it = result.begin() ; it != result.end(); ) {
    bool remove = false;
    hb_enc::hb_ptr hb = *it;
    if( hb->type == hb_enc::hb_t::rf ) {
      if( !helpers::exists( relaxed_wrs, hb->e2->prog_v ) && !hb->e2->is_rlx())
      // if( !hb->e1->is_rlx() && !hb->e2->is_rlx() )
      // if( !hb->e1->is_rlx() )
        remove = true;
    }
    if( remove ) it = result.erase(it); else ++it; 
  }
  return result;
}


