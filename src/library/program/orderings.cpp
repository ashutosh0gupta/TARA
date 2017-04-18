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
#include "hb_enc/hb.h"

namespace tara {

  void accesses( const hb_enc::se_ptr& ep,
                    const hb_enc::se_set& ep_sets,
                    hb_enc::se_set& new_prevs ) {
    if( ep->is_mem_op() )
      new_prevs.insert( ep );
    for( auto& epp : ep_sets )
      if( epp->is_mem_op() )
        new_prevs.insert( epp );
  }

  void dominate_wr_accesses( const hb_enc::se_ptr& ep,
                             const hb_enc::se_set& ep_sets,
                             hb_enc::se_set& new_prevs ) {
    if( ep->is_wr() )
      new_prevs.insert( ep );
    for( auto& epp : ep_sets )
      if( epp->is_wr() )
        if( !ep->is_wr() || !ep->access_same_var( epp ) )
          new_prevs.insert( epp );
  }

void program::update_seq_orderings() {
  if( seq_ordering_has_been_called )
    program_error( "update_seq_orderning must not be called twice!!" );
  seq_ordering_has_been_called = true;

  seq_dom_wr_before.clear();
  seq_rd_before.clear();
  seq_dom_wr_after.clear();

  // create solver
  std::vector< std::pair<hb_enc::se_ptr,hb_enc::se_ptr> > thread_syncs;

  all_es.push_back( init_loc );
  for( unsigned i = 0; i < size(); i++ ) {
    for( auto& e : get_thread(i).events ) {
      all_es.push_back(e);
      for( auto& ep : e->prev_events ) {
        ord_solver.add( _hb_encoding.mk_hbs(ep,e) );
      }
    }
    auto& se = get_thread(i).start_event;
    auto& fe = get_thread(i).final_event;
    all_es.push_back(se);
    all_es.push_back(fe);//todo: check if the final event is in (i).events
    for( auto& ep : get_thread(i).final_event->prev_events ) {
      ord_solver.add( _hb_encoding.mk_hbs(ep,fe) );
    }
    auto& ce = get_create_event(i);
    if( ce ) {
      ord_solver.add( _hb_encoding.mk_hbs(ce,se));
      thread_syncs.push_back({ce,se});
    }
    auto& je = get_join_event(i);
    if( je ) {
      ord_solver.add( _hb_encoding.mk_hbs(fe,je));
      thread_syncs.push_back({fe,je});
    }
  }
  if( post_loc ) all_es.push_back( post_loc );

  auto res = ord_solver.check();
  assert( res == z3::check_result::sat);
  auto m = ord_solver.get_model();

  std::map< hb_enc::se_ptr, int > order_map;
  for( auto e : all_es ) {
    auto v = m.eval( e->get_solver_symbol() );
    order_map[e] = _z3.get_numeral_int( v );
  }

  std::sort( all_es.begin(), all_es.end(),
             [&](const hb_enc::se_ptr& x, const hb_enc::se_ptr& y) {
               return order_map.at(x) < order_map.at(y);
               } );

  for( auto e : all_es ) {
    auto& prevs = seq_dom_wr_before[e];
    auto& rd_prevs = seq_rd_before[e];
    prevs.clear();
    for( auto& ep : e->prev_events ) {
      dominate_wr_accesses( ep, seq_dom_wr_before.at(ep), prevs );
      accesses( ep, seq_rd_before.at(ep), rd_prevs );
      // prevs.insert( ep ); //todo: include fences??
      // helpers::set_insert( prevs,seq_dom_wr_before.at(ep) );
    }
    for( const auto& sync : thread_syncs ) {
      if( sync.second != e ) continue;
      hb_enc::se_ptr ep = sync.first;
      dominate_wr_accesses( ep, seq_dom_wr_before.at(ep), prevs );
      accesses( ep, seq_rd_before.at(ep), rd_prevs );
      // prevs.insert( sync.first );
      // helpers::set_insert( prevs, seq_dom_wr_before.at(sync.first) );
    }
  }

  auto rit = all_es.rbegin(), rend = all_es.rend();
  for (; rit!= rend; ++rit) {
    hb_enc::se_ptr e = *rit;
    auto& prevs = seq_dom_wr_after[e];
    prevs.clear();
    for( auto& ep : e->post_events ) {
      dominate_wr_accesses( ep.e, seq_dom_wr_after.at(ep.e), prevs );
      // prevs.insert( ep.e );
      // helpers::set_insert( prevs,seq_dom_wr_after.at(ep.e) );
    }
    for( const auto& sync : thread_syncs ) {
      if( sync.first != e ) continue;
      hb_enc::se_ptr ep = sync.second;
      dominate_wr_accesses( ep, seq_dom_wr_after.at(ep), prevs );
      // prevs.insert( sync.second );
      // helpers::set_insert( prevs, seq_dom_wr_after.at(sync.second) );
    }
  }


}

}
