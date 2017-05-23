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

// "ARMv8 AArch64"

// Cross.cat
// (* Utilities for combining mo's *)

// (* Compute linarisations per locations *)
// let mo_locs (pmo,wss) =
//   let rec do_locs wss = match wss with
//   || {} -> {}
//   || ws ++ wss ->
//       linearisations(ws,pmo) ++ do_locs(wss)   
//   end in do_locs(wss)

// (* Cross product linearisations *)
// let cross = 
//   let rec do_cross (k,ys,oss) = match oss with
//   || {} -> ys ++ k
//   || os ++ oss ->
//        let rec call_rec (k,os) = match os with
//        || {} -> k
//        || o ++ os ->
//            call_rec (do_cross (k,o | ys,oss),os)
//        end in
//        call_rec (k,os)
//   end in
//   fun oss -> do_cross ({},0,oss)

// (* Generate mo's that extend partial order pmo *)
// let generate_orders(s,pmo) = cross (mo_locs (pmo,partition s))
// let generate_mos(pmo) = generate_orders(W,pmo)

// mos.cat

// let invrf = rf^-1
// let mobase = mo0
// with mo from generate_mos(mobase)

// (* From now, mo is a coherence order *)
// let moi = mo & int
// let moe = mo \ moi

// (* Compute fr *)
// let fr = (invrf ; mo) \ id
// let fri = fr & int
// let fre = fr \ fri

// show mo,fr

// =====================================


// (* Dependencies *)
// show data,addr
// let ctrlisb = try ctrlcfence(ISB) with 0 ???????
// show ctrlisb
// show isb \ ctrlisb as isb
// show ctrl \ ctrlisb as ctrl

// =====================================



// (* Internal visibility requirement *)
// acyclic po-loc | fr | mo | rf


// (* Barrier-ordered-before *)


// (* Atomic: Basic LDXR/STXR constraint to forbid intervening writes. *)
// empty rmw & (fre; moe)

// void wmm_event_cons::distinct_events_arm8_2() {

// }


// only answers to obvious orderings
bool wmm_event_cons::is_ordered_arm8_2( const hb_enc::se_ptr& e1,
                                         const hb_enc::se_ptr& e2  ) {
  if( e1->is_barr_type() || e2->is_barr_type() ) {
    return is_barrier_ordered( e1, e2 );
  }
  // bool find = false;
  assert( e1->is_mem_op() && e2->is_mem_op() );
  if( e1->is_mem_op() && e2->is_wr() &&
      (e1->prog_v.name == e2->prog_v.name) ) return true;

  if( e1->is_wr() ) return false;

  return false;
}

// symbolic hb must enforce the following
//: rfe | fre | moe | data; rfi | rmw; rfi
// rfi should not be added
// moi and fri can be added without concern??
// and coherence

// todo : borrowed from rmo
void wmm_event_cons::ppo_arm8_2( const tara::thread& thread ) {


// dob | aob | bob are summrized in the following

// L : store release
// A : load acquire
// Q : load acQuire

//-----------------------------------------------
// ppo
// 	| ctrl; [W]
// 	| ctrl; moi
//      | data
// 	| data; moi
//      | rmw
// 	| [L]; po; [A]
// 	| [A | Q]; po
// 	| po; [L]
// 	| po; [L]; moi

//----------------------------------------------
//  barrier ppo
//
//      | po; [dmb.full]; po
// 	| [R]; po; [dmb.ld]; po
// 	| [W]; po; [dmb.st]; po; [W]

//---------------------------------------------
// unsupported ppo
//      | addr
// 	| addr; rfi
// 	| ctrl;     [ISB]; po; [R]
// 	| addr; po; [ISB]; po; [R]
// 	| addr; po; [W]


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
        po = po && implies( dep.cond, hb_encoding.mk_ghbs( dep.e, e ));
        hb_enc::join_depends_set( dep, ordered );
      }else if( z3::expr cond = is_ordered_dependency( dep.e, e ) ) {
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
