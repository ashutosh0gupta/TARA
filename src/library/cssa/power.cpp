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

// com = rf | fr | co

// coherence
// (* sc per location *) acyclic po-loc | rf | fr | co

// (* fences *)
// let fence = RM(lwsync)|WW(lwsync)|sync

// (* no thin air *)
// let hb = ppo|fence|rfe
// acyclic hb

// (* prop *)
// let prop-base = (fence|(rfe;fence));hb*
// let prop = WW(prop-base)|(com*;prop-base*;sync;hb*)

// (* observation *) irreflexive fre;prop;hb*

// (* propagation *) acyclic co | prop

bool wmm_event_cons::is_ordered_power( const hb_enc::se_ptr& e1,
                                       const hb_enc::se_ptr& e2  ) {
  assert(false);
}

void wmm_event_cons::ppo_power( const tara::thread& thread ) {
  // (* ppo *)
  // let ii0    = addr | data | ( po-loc & (fre;rfe) ) |rfi
  // let ic0    = 0
  // let ci0    = (ctrl+isync)|( po-loc & (coe;rfe) )
  // let cc0    = addr| data| po-loc|ctrl|(addr;po)

  // let rec ii = ii0 | ci      | (ic;ci) | (ii;ii)
  // and     ic = ic0 | ii | cc | (ic;cc) | (ii;ic)
  // and     ci = ci0           | (ci;ii) | (cc;ci)
  // and     cc = cc0 | ci      | (ci;ic) | (cc;cc)
  // let ppo = RR(ii)|RW(ic)
  std::unordered_set<hb_enc::se_ptr> ev_set1,ev_set2;
  get_power_ii0( thread, ev_set1, ev_set2 );
  get_power_ci0( thread, ev_set1, ev_set2 );
  get_power_cc0( thread, ev_set1, ev_set2 );
  get_power_mutual_rec_cons( thread, ev_set1, ev_set2 );

  //hb_enc->mk_ghb( e1, e2 );
  //z3::expr prop_e1_e2 = hb_enc->mk_ghb_power_prop( e1, e2 );
  
  //p.unsupported_mm();
}

void wmm_event_cons::
get_power_ii0( const tara::thread& thread,
               std::unordered_set<hb_enc::se_ptr>& ev_set1,
               std::unordered_set<hb_enc::se_ptr>& ev_set2) {
  // let ii0    = addr | data | ( po-loc & (fre;rfe) ) |rfi
  for(auto rf_pair:rf_rel) {
    hb_enc::se_ptr e1=std::get<1>(rf_pair),e2=std::get<2>(rf_pair);
    if(e1->tid==e2->tid && e1->tid==thread.start_event->tid) {//rfi
      //add to ii0
      event_pair ev_pair(e1,e2);
      ii0.insert(std::make_pair(ev_pair,std::get<0>(rf_pair)));
      ev_set1.insert(e1);
      ev_set2.insert(e2);
    }
    if(e2->tid==thread.start_event->tid) {//( po-loc & (fre;rfe) )
      for(auto fr_pair:fr_rel) {
        if( std::get<2>(fr_pair)==e1&&
            std::get<1>(fr_pair)->tid==thread.start_event->tid &&
            is_po_new(std::get<1>(fr_pair),e2) ) {
          //add to ii0
          event_pair ev_pair(std::get<1>(fr_pair),e2);
          ii0.insert(std::make_pair(ev_pair,(std::get<0>(fr_pair))));
          ev_set1.insert(std::get<1>(fr_pair));
          ev_set2.insert(e2);
        }
      }
    }
  }
  for(hb_enc::se_ptr e:thread.events) { //data
    for(auto dep:e->data_dependency) {
      //add to ii0
      event_pair ev_pair(dep.e,e);
      ii0.insert(std::make_pair(ev_pair, e->guard && dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
  }
}

void wmm_event_cons::
get_power_ci0( const tara::thread& thread,
               std::unordered_set<hb_enc::se_ptr>& ev_set1,
               std::unordered_set<hb_enc::se_ptr>& ev_set2 ) {
  // let ci0    = (ctrl+isync)|( po-loc & (coe;rfe) )
  for(auto e:thread.events) {
    for(auto dep:e->ctrl_dependency) {
      //ci0.insert(std::make_tuple(z3.c.int_val(0),z3.mk_true(),dep.e,e));
    }
  }
  for(auto rf_pair:rf_rel) {
    hb_enc::se_ptr e1=std::get<1>(rf_pair),e2=std::get<2>(rf_pair);
    if(e1->tid!=e2->tid&&e2->tid==thread.start_event->tid) {
      for(auto ep:e2->prev_events) {
        if(0) {//ww coherence
        }
      }
    }
  }
}
void wmm_event_cons::
get_power_cc0( const tara::thread& thread,
               std::unordered_set<hb_enc::se_ptr>& ev_set1,
               std::unordered_set<hb_enc::se_ptr>& ev_set2)
{
  // let cc0    = addr| data| po-loc|ctrl|(addr;po)
  for(hb_enc::se_ptr e:thread.events) {
    for(auto dep:e->data_dependency) {//data
      //add to cc0
      event_pair ev_pair(dep.e,e);
      cc0.insert(std::make_pair(ev_pair,e->guard&&dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
    for(auto ep:e->prev_events){ //po-loc
      tara::variable var1(z3.c),var2(z3.c);
      if(ep->is_rd()) var1=ep->rd_v();
      else var1=ep->wr_v();
      if(e->is_rd()) var2=e->rd_v();
      else var2=e->wr_v();
      if(var1==var2) { //add to cc0
        event_pair ev_pair(ep,e);
        cc0.insert(std::make_pair(ev_pair,e->guard&&ep->guard));
        ev_set1.insert(ep);
        ev_set2.insert(e);
      }
    }
    for(auto dep:e->ctrl_dependency) { //ctrl
      //add to cc0
      event_pair ev_pair(dep.e,e);
      cc0.insert(std::make_pair(ev_pair,e->guard&&dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
  }
}

void wmm_event_cons::
get_power_mutual_rec_cons( const tara::thread& thread,
                           std::unordered_set<hb_enc::se_ptr> ev_set1,
                           std::unordered_set<hb_enc::se_ptr> ev_set2 ) {
  // let rec ii = ii0 | ci      | (ic;ci) | (ii;ii)
  // and     ic = ic0 | ii | cc | (ic;cc) | (ii;ic)
  // and     ci = ci0           | (ci;ii) | (cc;ci)
  // and     cc = cc0 | ci      | (ci;ic) | (cc;cc)
  rec_rel ii,ic,ci,cc;
  for(hb_enc::se_ptr e1:ev_set1) {
    for(hb_enc::se_ptr e2:ev_set2) {
      if(e1==e2) continue;
      if(e1->is_post()||e2->is_pre()) continue;
      if(e1->get_topological_order()>=e2->get_topological_order()) continue;
      //info ii_info,ic_info,ci_info,cc_info;

      z3::expr t=z3.c.int_const(("t_ii_"+e1->name()+"_"+e2->name()).c_str());
      z3::expr bit=z3.c.bool_const(("ii"+e1->name()+e2->name()).c_str());
      z3::expr cond=z3.mk_false();
      ii.insert(std::make_pair(event_pair(e1,e2),std::make_tuple(t,bit,cond)));

      t=z3.c.int_const(("t_ic_"+e1->name()+"_"+e2->name()).c_str());
      bit=z3.c.bool_const(("ic"+e1->name()+e2->name()).c_str());
      cond=z3.mk_false();
      ic.insert(std::make_pair(event_pair(e1,e2),std::make_tuple(t,bit,cond)));

      t=z3.c.int_const(("t_ci_"+e1->name()+"_"+e2->name()).c_str());
      bit=z3.c.bool_const(("ci"+e1->name()+e2->name()).c_str());
      cond=z3.mk_false();
      ci.insert(std::make_pair(event_pair(e1,e2),std::make_tuple(t,bit,cond)));

      t=z3.c.int_const(("t_cc_"+e1->name()+"_"+e2->name()).c_str());
      bit=z3.c.bool_const(("cc"+e1->name()+e2->name()).c_str());
      cond=z3.mk_false();
      cc.insert(std::make_pair(event_pair(e1,e2),std::make_tuple(t,bit,cond)));
    }
  }
  for(hb_enc::se_ptr e1:ev_set1) {
    for(hb_enc::se_ptr e2:ev_set2) {
      if(e1==e2) continue;
      if(e1->is_post()||e2->is_pre()) continue;
      if(e1->get_topological_order()>=e2->get_topological_order()) continue;
      event_pair ev_pair(e1,e2);
      z3::expr t=std::get<0>(ii.find(ev_pair)->second),
        b=std::get<1>(ii.find(ev_pair)->second),
        cond=std::get<2>(ii.find(ev_pair)->second);
      //insert into ii
      if(ii0.find(ev_pair)!=ii0.end()) {
        cond=cond||((t==0)&&ii0.find(ev_pair)->second);
      }
      z3::expr t_ci=std::get<0>(ci.find(ev_pair)->second),
        b_ci=std::get<1>(ci.find(ev_pair)->second),
        cond_ci=std::get<2>(ci.find(ev_pair)->second);
      cond=cond||((t>=t_ci)&&cond_ci&&b_ci);
    }
  }
}
