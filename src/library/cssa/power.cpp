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
  std::set<hb_enc::se_ptr> ev_set1,ev_set2;
  get_power_ii0(thread,ev_set1,ev_set2);
  get_power_ci0(thread,ev_set1,ev_set2);
  get_power_cc0(thread,ev_set1,ev_set2);
  get_power_mutual_rec_cons(thread,ev_set1,ev_set2);

  //hb_enc->mk_ghb( e1, e2 );
  //z3::expr prop_e1_e2 = hb_enc->mk_ghb_power_prop( e1, e2 );
  
  //p.unsupported_mm();
}

void wmm_event_cons::get_power_ii0(const tara::thread& thread,
                                   std::set<hb_enc::se_ptr>& ev_set1,
                                   std::set<hb_enc::se_ptr>& ev_set2) {
  // let ii0    = addr | data | ( po-loc & (fre;rfe) ) |rfi
  for(auto rf_pair:rf_rel) {
    hb_enc::se_ptr e1=std::get<1>(rf_pair),e2=std::get<2>(rf_pair);
    if(e1->tid==e2->tid && e1->tid==thread.start_event->tid) { //rfi
      //add to ii0
      event_pair ev_pair(e1,e2);
      ii0.insert(std::make_pair(ev_pair,std::get<0>(rf_pair)));
      ev_set1.insert(e1);
      ev_set2.insert(e2);
    }
    if(e2->tid==thread.start_event->tid) { //( po-loc & (fre;rfe) )
      for(auto fr_pair:fr_rel) {
        if(std::get<2>(fr_pair)==e1&&
           std::get<1>(fr_pair)->tid==thread.start_event->tid&&
           is_po_new(std::get<1>(fr_pair),e2)) {
          //add to ii0
          event_pair ev_pair(std::get<1>(fr_pair),e2);
          ii0.insert(std::make_pair(ev_pair,(std::get<0>(fr_pair))));
          ev_set1.insert(std::get<1>(fr_pair));
          ev_set2.insert(e2);
        }
      }
    }
  }
  for(hb_enc::se_ptr e:thread.events) {//data
    for(auto dep:e->data_dependency) {
      //add to ii0
      event_pair ev_pair(dep.e,e);
      ii0.insert(std::make_pair(ev_pair,e->guard&&dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
  }
}
void wmm_event_cons::get_power_ci0(const tara::thread& thread,
                                   std::set<hb_enc::se_ptr>& ev_set1,
                                   std::set<hb_enc::se_ptr>& ev_set2)
{
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
void wmm_event_cons::get_power_cc0(const tara::thread& thread,
                                   std::set<hb_enc::se_ptr>& ev_set1,
                                   std::set<hb_enc::se_ptr>& ev_set2) {
  // let cc0    = addr| data| po-loc|ctrl|(addr;po)
  for(hb_enc::se_ptr e:thread.events) {
    for(auto dep:e->data_dependency) {//data
      //add to cc0
      event_pair ev_pair(dep.e,e);
      cc0.insert(std::make_pair(ev_pair,e->guard&&dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
    for(auto ep:e->prev_events) {//po-loc
      tara::variable var1(z3.c),var2(z3.c);
      if(ep->is_rd()) var1=ep->rd_v();
      else var1=ep->wr_v();
      if(e->is_rd()) var2=e->rd_v();
      else var2=e->wr_v();
      if(var1==var2) {//add to cc0
        event_pair ev_pair(ep,e);
        cc0.insert(std::make_pair(ev_pair,e->guard&&ep->guard));
        ev_set1.insert(ep);
        ev_set2.insert(e);
      }
    }
    for(auto dep:e->ctrl_dependency) {//ctrl
      //add to cc0
      event_pair ev_pair(dep.e,e);
      cc0.insert(std::make_pair(ev_pair,e->guard&&dep.e->guard));
      ev_set1.insert(dep.e);
      ev_set2.insert(e);
    }
  }
}

void wmm_event_cons::initialize(rec_rel& r, hb_enc::se_ptr& e1,
								hb_enc::se_ptr& e2,std::string r_name){
	z3::expr t=z3.c.int_const(("t_"+r_name+"_"+e1->name()+"_"+e2->name()).c_str());
	z3::expr bit=z3.c.bool_const((r_name+e1->name()+e2->name()).c_str());
	z3::expr cond=z3.mk_false();
	r.insert(std::make_pair(event_pair(e1,e2),std::make_tuple(t,bit,cond)));
}

void wmm_event_cons::initialize_rels(rec_rel& ii, rec_rel& ic,
																			rec_rel& ci, rec_rel& cc,
																			std::set<hb_enc::se_ptr>& ev_set1,
																			std::set<hb_enc::se_ptr>& ev_set2) {
	for(hb_enc::se_ptr e1:ev_set1) {
	  for(hb_enc::se_ptr e2:ev_set2) {
	    if(e1==e2) continue;
	    if(e1->is_post()||e2->is_pre()) continue;
	    if(e1->get_topological_order()>=e2->get_topological_order()) continue;

	    initialize(ii,e1,e2,"ii");
	    initialize(ic,e1,e2,"ic");
	    initialize(ci,e1,e2,"ci");
	    initialize(cc,e1,e2,"cc");
	  }
	}
}

z3::expr wmm_event_cons::get_rel_cond(rec_rel& r, event_pair& ev_pair, z3::expr& t) {
	z3::expr t_r=std::get<0>(r.find(ev_pair)->second),
	b_r=std::get<1>(r.find(ev_pair)->second),
	cond_r=std::get<2>(r.find(ev_pair)->second);
	return (t>=t_r)&&cond_r&&b_r;
}

void wmm_event_cons::get_power_mutual_rec_cons(const tara::thread& thread,
                                               std::set<hb_enc::se_ptr>& ev_set1,
                                               std::set<hb_enc::se_ptr>& ev_set2) {
  // let rec ii = ii0 | ci      | (ic;ci) | (ii;ii)
  // and     ic = ic0 | ii | cc | (ic;cc) | (ii;ic)
  // and     ci = ci0           | (ci;ii) | (cc;ci)
  // and     cc = cc0 | ci      | (ci;ic) | (cc;cc)
  rec_rel ii,ic,ci,cc;
  //initialization of ii, ic, ci, cc relations
  initialize_rels(ii,ic,ci,cc,ev_set1,ev_set2);

  //create constraints for each possible pair (e1,e2) in (ev_set1 X ev_set2)
  for(hb_enc::se_ptr e1:ev_set1) {
    for(hb_enc::se_ptr e2:ev_set2) {
    	//make sure (e1,e2) does not violate PO
      if(e1==e2) continue;
      if(e1->is_post()||e2->is_pre()) continue;
      if(e1->get_topological_order()>=e2->get_topological_order()) continue;

      event_pair ev_pair(e1,e2);
      //get (e1,e2) from ii
      z3::expr t0ii=std::get<0>(ii.find(ev_pair)->second),
        b0ii=std::get<1>(ii.find(ev_pair)->second),
        cond0ii=std::get<2>(ii.find(ev_pair)->second);
      //get (e1,e2) from ic
      z3::expr t0ic=std::get<0>(ic.find(ev_pair)->second),
        b0ic=std::get<1>(ic.find(ev_pair)->second),
        cond0ic=std::get<2>(ic.find(ev_pair)->second);
      //get (e1,e2) from ci
      z3::expr t0ci=std::get<0>(ci.find(ev_pair)->second),
        b0ci=std::get<1>(ci.find(ev_pair)->second),
        cond0ci=std::get<2>(ci.find(ev_pair)->second);
      //get (e1,e2) from cc
      z3::expr t0cc=std::get<0>(cc.find(ev_pair)->second),
        b0cc=std::get<1>(cc.find(ev_pair)->second),
        cond0cc=std::get<2>(cc.find(ev_pair)->second);

      //various cases in recursive definition
      //update conditions for existence of (e1,e2) in ii, ic, ci, cc
      if(ii0.find(ev_pair)!=ii0.end()) {//ii0 in ii
        cond0ii=cond0ii||((t0ii==0)&&ii0.find(ev_pair)->second);
      }
      if(ci0.find(ev_pair)!=ci0.end()) {//ci0 in ci
        cond0ci=cond0ci||((t0ci==0)&&ci0.find(ev_pair)->second);
      }
      if(cc0.find(ev_pair)!=cc0.end()) {//cc0 in cc
        cond0cc=cond0cc||((t0cc==0)&&cc0.find(ev_pair)->second);
      }

      cond0ii=cond0ii||get_rel_cond(ci,ev_pair,t0ii);//ci in ii
      cond0ic=cond0ic||get_rel_cond(ii,ev_pair,t0ic);//ii in ic
      cond0ic=cond0ic||get_rel_cond(cc,ev_pair,t0ic);//cc in ic
      cond0cc=cond0cc||get_rel_cond(ci,ev_pair,t0cc);//ci in cc

      //derive new pairs by joining existing pairs
      for(hb_enc::se_ptr eb:e1->prev_events) {
        if(ev_set1.find(eb)!=ev_set1.end()&&ev_set2.find(eb)!=ev_set2.end()) {
        	{//(ic;ci) in ii
          	assert(ic.find(std::make_pair(e1,eb))!=ic.end()&&
          				 ci.find(std::make_pair(eb,e2))!=ci.end());
          	z3::expr t_ic=std::get<0>(ic.find(ev_pair)->second),
          		b_ic=std::get<1>(ic.find(ev_pair)->second),
            	cond_ic=std::get<2>(ic.find(ev_pair)->second);
          	z3::expr t_ci=std::get<0>(ci.find(ev_pair)->second),
          		b_ci=std::get<1>(ci.find(ev_pair)->second),
            	cond_ci=std::get<2>(ci.find(ev_pair)->second);
          	cond0ii=cond0ii||
          		((t0ii>=t_ic)&&(t0ii>=t_ci)&&b_ic&&b_ci&&cond_ic&&cond_ci);
        	}
          {//(ii;ii) in ii
          	assert(ii.find(std::make_pair(e1,eb))!=ic.end()&&
          				 ii.find(std::make_pair(eb,e2))!=ci.end());
          	z3::expr t_ii1=std::get<0>(ii.find(ev_pair)->second),
          		b_ii1=std::get<1>(ii.find(ev_pair)->second),
							cond_ii1=std::get<2>(ii.find(ev_pair)->second);
          	z3::expr t_ii2=std::get<0>(ii.find(ev_pair)->second),
          		b_ii2=std::get<1>(ii.find(ev_pair)->second),
							cond_ii2=std::get<2>(ii.find(ev_pair)->second);
          	cond0ii=cond0ii||((t0ii>=t_ii1)&&(t0ii>=t_ii2)&&
                            b_ii1&&b_ii2&&cond_ii1&&cond_ii2);
        	}
          {//(ic;cc) in ic
            assert(ic.find(std::make_pair(e1,eb))!=ic.end()&&
            			 cc.find(std::make_pair(eb,e2))!=ci.end());
            z3::expr t_ic=std::get<0>(ic.find(ev_pair)->second),
              b_ic=std::get<1>(ic.find(ev_pair)->second),
              cond_ic=std::get<2>(ic.find(ev_pair)->second);
            z3::expr t_cc=std::get<0>(cc.find(ev_pair)->second),
              b_cc=std::get<1>(cc.find(ev_pair)->second),
              cond_cc=std::get<2>(cc.find(ev_pair)->second);
            cond0ic=cond0ii||((t0ic>=t_ic)&&(t0ii>=t_cc)&&
                              b_ic&&b_cc&&cond_ic&&cond_cc);
          }
        }
      }
    }
  }
}
