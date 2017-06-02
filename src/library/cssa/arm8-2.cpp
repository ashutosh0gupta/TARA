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
 *
 */

#include "wmm.h"
#include "helpers/helpers.h"
#include "helpers/z3interf.h"
#include "hb_enc/hb.h"
#include <assert.h>

using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

// The model is borrowed from Herd7 infrastructure
//
// https://github.com/herd/herdtools7/blob/master/herd/libdir/aarch64.cat
// version: daa126680b6ecba97ba47b3e05bbaa51a89f27b7
//


// coherence and rmw conditions are handelled by wmm.cpp
// acyclic po-loc | fr | mo | rf
// empty rmw & (fre; moe)

//----------------------------------------------------------------------------

// todo: revisit the following ordering attributes
// L : store release
// A : load acquire
// Q : load acQuire

bool is_L( const hb_enc::se_ptr e ) {
  // return e->is_wr() && e->is_sc();
  return e->is_wr() && e->is_rls();
}

bool is_A( const hb_enc::se_ptr e ) {
  return e->is_rd() && e->is_acq();
}

bool is_Q( const hb_enc::se_ptr e ) {
  return e->is_rd() && e->is_sc();
}

bool is_dmb_full( const hb_enc::se_ptr e ) {
  return e->is_fence() || e->is_thread_create() || e->is_thread_join();
}

bool is_dmb_ld( const hb_enc::se_ptr e ) {
  return e->is_fence_a();
}

bool is_dmb_st( const hb_enc::se_ptr e ) {
  return e->is_fence_b();
}


// only answers to obvious orderings
// called only if (e1,e2) \in po
//todo:
//  ** optimize fence interactions
//  *** rmw handling (do we need different time stamps??)
//  *** moi,fri are included in hb
//  *** rfi is not added
//  *** thread create/join inserts dmb.full in the caller thread


// symbolic hb must enforce the following
//: rfe | fre | moe | data; rfi | rmw; rfi
// unsupported
// 	| addr; rfi
// rfi should not be added
// and coherence


bool wmm_event_cons::is_ordered_arm8_2( const hb_enc::se_ptr& e1,
                                        const hb_enc::se_ptr& e2  ) {
  if( e1->is_block_head() || e2->is_block_head() ) return false;
// 	| [L]; po; [A]
  if( is_L(e1) && is_A(e2) ) return true;
// 	| Q; po
  if( is_Q(e1) && e2->is_mem_op() ) return true;
// 	| A; po
  if( is_A(e1) && e2->is_mem_op() ) return true;
// 	| po; [L]
  if( e1->is_mem_op() && is_L(e2) ) return true;
//      | po; [dmb.full]; po
  if( is_dmb_full(e1) && (e2->is_mem_op()||is_dmb_full(e2)) ) return true;
  if( (e1->is_mem_op()||is_dmb_full(e1)) && is_dmb_full(e2) ) return true;
// 	| [R]; po; [dmb.ld]; po
  if( is_dmb_ld(e1) && (e2->is_mem_op()||is_dmb_ld(e2)) ) return true;
  if( (e1->is_rd()||is_dmb_ld(e1)) && is_dmb_ld(e2)  ) return true;
// 	| [W]; po; [dmb.st]; po; [W]
  if( is_dmb_st(e1) && (e2->is_wr()||is_dmb_st(e2)) ) return true;
  if( (e1->is_wr()||is_dmb_st(e1)) && is_dmb_st(e2)  ) return true;

//      | rmw
  if( e2->is_wr() && e1 == e2->rmw_other ) return true;

// unsupported
// 	| ctrl;     [ISB]; po; [R]
// 	| addr; po; [ISB]; po; [R]
// 	| addr; po; [W]


  //moi added in ppo
  // However, the model defines it as follows
  // | po; [L]; moi
  // | ctrl   ; moi
  // | data   ; moi
  if( e1->is_wr() && e2->is_wr() && e1->access_same_var(e2) ) return true;

  return false;

}


z3::expr wmm_event_cons::is_dependency_arm8_2( const hb_enc::se_ptr& e1,
                                               const hb_enc::se_ptr& e2  ) {

  if( !(e1->is_mem_op() && e2->is_mem_op()) ) return z3.mk_emptyexpr();

  // | data
  z3::expr cond = e2->get_data_dependency_cond( e1 );
  // | ctrl; [W]
  if( e2->is_wr() ) {
    z3::expr ctrl_cond = e2->get_ctrl_dependency_cond( e1 );
    if( ctrl_cond ) {
      if( cond ) {
        cond = ctrl_cond || cond;
      }else{
        cond = ctrl_cond;
      }
    }
  }
  // unsupported
  // | addr

  return cond;
}


// rf_b for wr -> rd
// cond,e \in wr.dep
// e ord r
z3::expr wmm_event_cons::rfi_ord_arm8_2( z3::expr rf_b,
                                         const hb_enc::se_ptr& wr,
                                         const hb_enc::se_ptr& rd ) {
  if( !p.is_mm_arm8_2() ) return z3.mk_true();
  if( wr->tid != rd->tid ) return z3.mk_true();

  assert( wr->tid == rd->tid );
  // |data; rfi
  z3::expr dep_rfis = z3.mk_true();
  for( auto& dep : wr->data_dependency ) {
    dep_rfis = dep_rfis &&
      z3::implies( dep.cond && rf_b && wr->guard,
                   hb_encoding.mk_ghbs( dep.e, rd ) );
  }
  //| rmw; rfi
  if( auto& rmw_rd = wr->rmw_other ) {
    dep_rfis = dep_rfis && z3::implies(rf_b, hb_encoding.mk_ghbs(rmw_rd, rd));
  }
  // for( const auto& pr : p.split_rmws ) { //todo: ineffcient search
  //   if( wr == pr.second ) {
  //     const auto& rmw_rd = pr.first;
  //     assert( rmw_rd->is_rd() );
  //   }
  // }
  // if( wr->is_update() ) {
  //   dep_rfis = dep_rfis &&
  //     z3::implies( rf_b, hb_encoding.mk_ghbs( wr, rd ) );
  // }

  // (unsupported)
  //|  addr; rfi
  return dep_rfis;
}

void wmm_event_cons::ppo_arm8_2( const tara::thread& thread ) {


  hb_enc::se_to_depends_map ordered_map,pending_map;
  auto& se = thread.start_event;
  pending_map[se].insert( hb_enc::depends( se, z3.mk_true() ) );

  for( auto& e : thread.events ) {
    pending_map[e].insert( {e , z3.mk_true() } );
    hb_enc::depends_set tmp_pendings;
    for( auto& ep : e->prev_events )
      hb_enc::join_depends_set( pending_map[ep], tmp_pendings );
    hb_enc::depends_set& pending = pending_map[e];
    hb_enc::depends_set& ordered = ordered_map[e];
    while( !tmp_pendings.empty() ) {
      //todo: does ordered access solve the problem
      auto dep = hb_enc::pick_maximal_depends_set( tmp_pendings );
      if( is_ordered_arm8_2( dep.e, e ) ) {
        // std::cout << "adding hb " << dep.e->name() << ","<< e->name() << "\n";
        po = po && implies( dep.cond, hb_encoding.mk_ghbs( dep.e, e ));
        hb_enc::join_depends_set( dep, ordered );
      }else if( z3::expr cond = is_dependency_arm8_2( dep.e, e ) ) {
        z3::expr c = cond && dep.cond;
        c = c.simplify();
      // tara::debug_print( std::cerr, c );
        if( !z3.is_false( c ) ) {
          po = po && implies( c, hb_encoding.mk_ghbs( dep.e, e ) );
          hb_enc::join_depends_set( dep.e, c, ordered );
        }
        if( !z3.is_true( c ) ) {
          hb_enc::join_depends_set( dep.e, dep.cond && !cond, pending );
          for( auto& depp : ordered_map[dep.e] )
            hb_enc::join_depends_set(depp.e,depp.cond && !cond, tmp_pendings);
        }
      }else{
        pending.insert( dep );
        hb_enc::join_depends_set( ordered_map[dep.e], tmp_pendings );
      }
    }
  }

  auto& fe = thread.final_event;
  for( auto& e : fe->prev_events ) {
    for( auto& dep : pending_map[e] ) {
      po = po && implies( dep.cond, hb_encoding.mk_ghbs( dep.e, fe ) );
    }
  }
}
