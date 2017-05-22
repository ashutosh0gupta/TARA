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

// (*
//  * Include aarch64fences.cat so that barriers show up in generated diagrams.
//  *)
// include "aarch64fences.cat"
// (* Fences *)
// let dmb.ish = try fencerel(DMB.ISH) with 0
// let dmb.ishld = try fencerel(DMB.ISHLD) with 0
// let dmb.ishst = try fencerel(DMB.ISHST) with 0
// let dmb.fullsy = try fencerel(DMB.SY) with 0
// let dmb.fullst = try fencerel(DMB.ST) with 0
// let dmb.fullld = try fencerel(DMB.LD) with 0
// let dmb.sy = dmb.fullsy | dmb.ish
// let dmb.st = dmb.fullst | dmb.ishst
// let dmb.ld = dmb.fullld | dmb.ishld
// let dsb.sy = try fencerel(DSB.SY) with 0
// let dsb.st = try fencerel(DSB.ST) with 0
// let dsb.ld = try fencerel(DSB.LD) with 0
// let isb = try fencerel(ISB) with 0
// show dmb.sy,dmb.st,dmb.ld,dsb.sy,sb.st,dsb.ld,dmb,dsb

// (* Dependencies *)
// show data,addr
// let ctrlisb = try ctrlcfence(ISB) with 0
// show ctrlisb
// show isb \ ctrlisb as isb
// show ctrl \ ctrlisb as ctrl

// =====================================

// (*
//  * As a restriction of the model, all observers are limited to the same
//  * inner-shareable domain. Consequently, the ISH, OSH and SY barrier
//  * options are all equivalent to each other.
//  *)
// let dsb.full = DSB.ISH | DSB.OSH | DSB.SY
// let dsb.ld = DSB.ISHLD | DSB.OSHLD | DSB.LD
// let dsb.st = DSB.ISHST | DSB.OSHST | DSB.ST

// (*
//  * A further restriction is that standard litmus tests are unable to
//  * distinguish between DMB and DSB instructions, so the model treats
//  * them as equivalent to each other.
//  *)
// let dmb.full = DMB.ISH | DMB.OSH | DMB.SY | dsb.full
// let dmb.ld = DMB.ISHLD | DMB.OSHLD | DMB.LD | dsb.ld
// let dmb.st = DMB.ISHST | DMB.OSHST | DMB.ST | dsb.st

// (* Flag any use of shareability options, due to the restrictions above. *)
// flag ~empty (dmb.full | dmb.ld | dmb.st) \
// 	    (DMB.SY | DMB.LD | DMB.ST | DSB.SY | DSB.LD | DSB.ST)
// 	    as Assuming-common-inner-shareable-domain


// (* Internal visibility requirement *)
// acyclic po-loc | fr | mo | rf as internal

// (* Dependency-ordered-before *)
// let dob =
//      | data
// 	| ctrl; [W]
// 	| (ctrl | data); moi
// 	| data; rfi

//---------------------------------------------
// unsupported
// 	| addr; rfi
// 	| (ctrl | (addr; po)); [ISB]; po; [R]
//      | addr
// 	| addr; po; [W]

// (* Atomic-ordered-before *)
// let aob = rmw | rmw; rfi

// (* Barrier-ordered-before *)
// let bob = po; [dmb.full]; po
// 	| [L]; po; [A]
// 	| [R]; po; [dmb.ld]; po
// 	| [A | Q]; po
// 	| [W]; po; [dmb.st]; po; [W]
// 	| po; [L]
// 	| po; [L]; moi

// acyclic rfe | fre | moe | dob | aob | bob as internal

// (* Atomic: Basic LDXR/STXR constraint to forbid intervening writes. *)
// empty rmw & (fre; moe) as atomic

void wmm_event_cons::distinct_events_arm8_2() {

}

void wmm_event_cons::ses_arm8_2() {

  //

}


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

// todo : borrowed from rmo
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
      if( is_ordered_rmo( dep.e, e ) ) {
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
