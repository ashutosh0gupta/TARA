/*
 * Copyright 2017, TIFR
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

// eager calculation of porgram order

#include "helpers/helpers.h"
#include "program/program.h"

namespace tara {

void program::update_seq_orderings() {
  seq_before.clear();
  seq_after.clear();

  if( init_loc ) seq_before[init_loc].clear();
  if( post_loc ) seq_before[post_loc].clear();
  for( unsigned i = 0; i < size(); i++ ) {
    seq_before[get_thread(i).start_event].clear();
    for( auto& e : get_thread(i).events ) {
      auto& sb_es = seq_before[e];
      for( auto& ep : e->prev_events ) {
        // if( ep->is_mem_op() )
        sb_es.insert( ep ); //todo: include fences??
        helpers::set_insert( sb_es,seq_before.at(ep) );
      }
    }
    auto& sb_es = seq_before[get_thread(i).final_event];
    for( auto& ep : get_thread(i).final_event->prev_events ) {
      // if( ep->is_mem_op() )
        sb_es.insert( ep ); //todo: include fences??
        helpers::set_insert( sb_es,seq_before.at(ep));
    }
  }

  if( init_loc ) seq_after[init_loc].clear();
  if( post_loc ) seq_after[post_loc].clear();
  for( unsigned i = 0; i < size(); i++ ) {
    seq_after[get_thread(i).final_event].clear();
    auto rit = get_thread(i).events.rbegin();
    auto rend = get_thread(i).events.rend();
    for (; rit!= rend; ++rit) {
      hb_enc::se_ptr e = *rit;
      auto& sa_es = seq_after[e];
      for( auto& ep : e->post_events ) {
        // if( ep.e->is_mem_op() )
        sa_es.insert( ep.e ); //todo: include fences??
        helpers::set_insert( sa_es,  seq_after[ep.e]);
      }
    }
    auto& sa_es = seq_after[get_thread(i).start_event];
    for( auto& ep : get_thread(i).start_event->post_events ) {
      // if( ep.e->is_mem_op() )
      sa_es.insert( ep.e ); //todo: include fences??
      helpers::set_insert( sa_es,  seq_after[ep.e]);
      // tara::debug_print( std::cerr, sa_es );
    }
  }

}

}
