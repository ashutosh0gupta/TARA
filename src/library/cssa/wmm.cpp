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

#include "wmm.h"
#include "helpers/helpers.h"
#include <string.h>
#include "helpers/z3interf.h"
#include "hb_enc/hb.h"
#include <utility>
#include <assert.h>
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

//----------------------------------------------------------------------------
// New code
//----------------------------------------------------------------------------

wmm_event_cons::wmm_event_cons( helpers::z3interf& _z3,
                                api::options& _o,
                                hb_enc::encoding& _hb_encoding,
                                tara::program& _p )
: z3( _z3 )
,o( _o )
,hb_encoding( _hb_encoding )
,p( _p ) {
}


bool wmm_event_cons::is_po( hb_enc::se_ptr x, hb_enc::se_ptr y ) {
  if( x == y )
    return true;
  if( x->is_pre() || y->is_post() ) return true;
  if( x->is_post() || y->is_pre() ) return false;
  if( x->tid != y->tid ) return false;
  if( x->get_topological_order() >= y->get_topological_order() ) return false;

  // return is_po_new( x, y);
  return p.is_seq_before( x, y );
}

//----------------------------------------------------------------------------
// memory model utilities


bool wmm_event_cons::in_grf( const hb_enc::se_ptr& wr,
                             const hb_enc::se_ptr& rd ) {
  if( p.is_mm_sc() ) {
    return true;
  }else if( p.is_mm_tso() || p.is_mm_pso() || p.is_mm_power() ||
            p.is_mm_rmo() || p.is_mm_alpha() || p.is_mm_arm8_2()) {
    return wr->tid != rd->tid; //only events with diff threads are part of grf
  }else{
    p.unsupported_mm();
    return false;
  }
}

bool wmm_event_cons::is_no_thin_mm() const {
  if( p.is_mm_sc() || p.is_mm_arm8_2() ) {
    return false;
  }else if(    p.is_mm_tso() // todo: tso/pso do not need thin air
            || p.is_mm_pso()
            || p.is_mm_rmo()
            || p.is_mm_alpha() ) {
    return true;
  }else{
    p.unsupported_mm();
    return false; // dummy return
  }
}

//----------------------------------------------------------------------------
// coherence preserving code
// po-loc must be compatible with
// n  - nothing to do
// r  - relaxed
// ne - not enforced
// po-loc needed to look at reversed
// (pp-loc)/(ses)
//          rel-type  po-ord sc   tso   pso   rmo  alpha  arm8.2
//  - ws      (w->w)   w->w  n    n     n     n     n      n
//  - fr      (r->w)   w->r  n    r     r     r     r      r
//  - fr;rf   (r->r)   r->r  n    n     n     r(ne) n      r
//  - rf      (w->r)   r->w  n    n     n     n     n      r
//  - ws;rf   (w->r)   r->w  n    n     n     n     n      r


bool wmm_event_cons::is_rd_rd_coherence_preserved() {
  if( p.is_mm_sc()
      || p.is_mm_tso()
      || p.is_mm_pso()
      || p.is_mm_alpha()
      || p.is_mm_arm8_2()
      ) {
    return true;
  }else if( p.is_mm_rmo() ){
    return false;
  }else{
    p.unsupported_mm();
    return false; // dummy return
  }
}

//----------------------------------------------------------------------------
// Build symbolic event structure
// In original implementation this part of constraints are
// referred as pi constraints

z3::expr wmm_event_cons::get_rf_bvar( const tara::variable& v1,
                                      hb_enc::se_ptr wr, hb_enc::se_ptr rd,
                                      bool record ) {
  z3::expr b = hb_encoding.get_rf_bit( v1, wr, rd );
  std::string bname = z3.get_top_func_name(b);
  // std::string bname = v1+"-"+wr->name()+"-"+rd->name();
  // z3::expr b = z3.c.bool_const(  bname.c_str() );
  if( record ) {
    p.reading_map.insert( std::make_tuple(bname, wr, rd) );
    if( p.is_mm_c11() ) // keep a copy in encoding object//todo: remove copy
      hb_encoding.rf_map.insert( std::make_tuple(bname, wr, rd) );
  }
  return b;
}



//----------------------------------------------------------------------------
bool wmm_event_cons::anti_po_read( const hb_enc::se_ptr& wr,
                                   const hb_enc::se_ptr& rd ) {
  // preventing coherence violation - rf
  // (if rf is local then may not visible to global ordering)
  assert( wr->tid >= p.size() || rd->tid >= p.size() ||
          wr->prog_v.name == rd->prog_v.name );
  if( p.is_mm_sc()
      || p.is_mm_tso()
      || p.is_mm_pso()
      || p.is_mm_rmo()
      || p.is_mm_alpha()
      || p.is_mm_c11()
      || p.is_mm_arm8_2() ) {
    // should come here for those memory models where rd-wr on
    // same variables are in ppo
    if( is_po_new( rd, wr ) ) {
      return true;
    }
    //
  }else{
    p.unsupported_mm();
  }
  return false;
}


bool wmm_event_cons::anti_po_loc_fr( const hb_enc::se_ptr& rd,
                                     const hb_enc::se_ptr& wr ) {
  // preventing coherence violation - fr;
  // (if rf is local then it may not be visible to the global ordering)
  // coherance disallows rf(rd,wr') and ws(wr',wr) and po-loc( wr, rd)
  assert( wr->tid >= p.size() || rd->tid >= p.size() ||
          wr->prog_v.name == rd->prog_v.name );
  if( p.is_mm_sc()
      || p.is_mm_tso()
      || p.is_mm_pso()
      || p.is_mm_rmo()
      || p.is_mm_alpha()
      || p.is_mm_arm8_2()
      ) {
    if( wr->is_update() && rd == wr ) return false;
    if( is_po_new( wr, rd ) ) {
      return true;
    }
    //
  }else{
    p.unsupported_mm();
  }
  return false;
}

bool wmm_event_cons::is_wr_wr_coherence_needed() {
  if( p.is_mm_sc()
      || p.is_mm_tso()
      || p.is_mm_pso()
      || p.is_mm_rmo()
      || p.is_mm_alpha()
      || p.is_mm_arm8_2() ) {
    return false;
  }else{
    p.unsupported_mm();
  }
  return false;//dummy return
}


bool wmm_event_cons::is_rd_wr_coherence_needed() {
  if( p.is_mm_sc()
      || p.is_mm_tso()
      || p.is_mm_pso()
      || p.is_mm_rmo()
      || p.is_mm_alpha() ) {
    return false;
  } if( p.is_mm_arm8_2() ) {
    return true;
  }else{
    p.unsupported_mm();
  }
  return false;//dummy return
}


void wmm_event_cons::ses() {
  // For each global variable we need to construct
  // - wf  well formed
  // - rf  read from
  // - grf global read from
  // - fr  from read
  // - ws  write serialization

// we assume wr-wr coherence (po o ws) is not needed
  assert(!is_wr_wr_coherence_needed());// check against new mm

  //todo: disable the following code if rd rd coherence not preserved
  //todo: remove the following. seq_before also calculates the following info
  hb_enc::se_to_ses_map prev_rds;
  for( unsigned t = 0; t < p.size(); t++ ) {
     const auto& thr = p.get_thread( t );
  // for( const auto& thr : p.threads ) {
    for( auto e : thr.events ) {
      hb_enc::se_set& rds = prev_rds[e];
      for( auto ep : e->prev_events ) {
        rds.insert( prev_rds[ep].begin(), prev_rds[ep].end() );
        if( ep->is_rd() ) rds.insert( ep );
      }
    }
  }

  for( const auto& v1 : p.globals ) {
    const auto& rds = p.rd_events[v1];
    const auto& wrs = p.wr_events[v1];
    for( const hb_enc::se_ptr& rd : rds ) {
      z3::expr some_rfs = z3.c.bool_val(false);
      z3::expr rd_v = rd->get_rd_expr(v1);
      for( const hb_enc::se_ptr& wr : wrs ) {
        // avoiding cycles po o rf
        if( anti_po_read( wr, rd ) ) continue;
        z3::expr wr_v = wr->get_wr_expr( v1 );
        z3::expr b = get_rf_bvar( v1, wr, rd );
        some_rfs = some_rfs || b;
        z3::expr eq = ( rd_v == wr_v );
        // well formed
        wf = wf && implies( b, wr->guard && eq);
        // read from
        rf_rel.insert(std::make_tuple(b,wr,rd));
        z3::expr new_rf = implies( b, hb_encoding.mk_ghbs( wr, rd ) );
        rf = rf && new_rf;
        if( is_no_thin_mm() ) {
          z3::expr new_thin = implies( b, hb_encoding.mk_ghb_thin( wr, rd ) );
          thin = thin && new_thin;
        }
        //global read from
        if( in_grf( wr, rd ) ) grf = grf && new_rf;
        if( p.is_mm_arm8_2() ) grf = grf && rfi_ord_arm8_2( b, wr, rd );
        // from read
        for( const hb_enc::se_ptr& after_wr : wrs ) {
          if( after_wr->name() != wr->name() ) {
            auto cond = b && hb_encoding.mk_ghbs(wr,after_wr)
                          && after_wr->guard;
            if( anti_po_loc_fr( rd, after_wr ) ) {
              //write-read coherence: avoiding cycles po o fr
              fr = fr && !cond;
            }else if( is_po_new( rd, after_wr ) ) {
              //write-read coherence: avoiding cycles po o (ws;rf)
              // if( is_rd_wr_coherence_needed() )
                fr = fr && implies( b, hb_encoding.mk_ghbs(wr,after_wr) );
            }else{
              auto new_fr = hb_encoding.mk_ghbs( rd, after_wr );
              if( is_rd_rd_coherence_preserved() ) {
                // read-read coherence, avoding cycles po U (fr o rf)
                for( auto before_rd : prev_rds[rd] ) {
                  if( rd->access_same_var( before_rd ) ) {
                    z3::expr anti_coherent_b =
                      get_rf_bvar( v1, after_wr, before_rd, false );
                    new_fr = !anti_coherent_b && new_fr;
                  }
                }
              }
              if( auto& rmw_w = rd->rmw_other ) {
                // rmw & (fr o mo) empty
                new_fr = new_fr && hb_encoding.mk_ghbs( rmw_w, after_wr );
              }
              fr_rel.insert(std::make_tuple(cond,rd,after_wr));
              fr = fr && implies( cond , new_fr );
            }
          }
        }
      }
      wf = wf && implies( rd->guard, some_rfs );
    }

    auto it1 = wrs.begin();
    for( ; it1 != wrs.end() ; it1++ ) {
      const hb_enc::se_ptr& wr1 = *it1;
      auto it2 = it1;
      it2++;
      for( ; it2 != wrs.end() ; it2++ ) {
        const hb_enc::se_ptr& wr2 = *it2;
        // write serialization
        // todo: what about ws;rf
        if( wr1->tid != wr2->tid && // Why this condition?
            !wr1->is_pre() && !wr2->is_pre() // no initializations
            ) {
          ws = ws && ( hb_encoding.mk_ghbs( wr1, wr2 ) ||
                       hb_encoding.mk_ghbs( wr2, wr1 ) );
        }
        //Coherence order or
        if(wr1->wr_v()==wr2->wr_v()) {
        	if(is_po_new(wr1,wr2)) {//acyclic (po o co) in SC per loc
        		co=co&&hb_encoding.mk_ghbs(wr1,wr2);
        	}
        	else {
        		z3::expr co_var=z3.c.bool_const(("co"+wr1->name()+"_"+wr2->name()).c_str())
        		coe_rel.insert(co_var,wr1,wr2);
        		co=co&&implies(co_var,hb_encoding.mk_ghbs(wr1,wr2));
        	}
        }
      }
    }

    if( is_no_thin_mm() ) {
      // dependency
      for( const hb_enc::se_ptr& wr : wrs ) {
        for( auto& dep : wr->data_dependency )
          thin = thin && z3::implies(dep.cond,hb_encoding.mk_hb_thin(dep.e,wr));
        // for( auto& dep : wr->ctrl_dependency )
        //   thin = thin && z3::implies(dep.cond, hb_encoding.mk_hb_thin( dep.e, wr ));
      }
      // for( const hb_enc::se_ptr& rd : rds ) {
      //   for( auto& dep : rd->ctrl_dependency )
      //     thin = thin && z3::implies(dep.cond, hb_encoding.mk_hb_thin( dep.e, rd ));
    // }
    }
  }
}




//----------------------------------------------------------------------------
// declare all events happen at different time points

void wmm_event_cons::distinct_events() {

  z3::expr_vector es( z3.c );

  es.push_back( p.init_loc->get_solver_symbol() );

  if( p.post_loc )
    es.push_back( p.post_loc->get_solver_symbol() );

  for( unsigned t=0; t < p.size(); t++ ) {
    auto& thread = p.get_thread(t);
    for(auto& e : thread.events )
      es.push_back( e->get_solver_symbol() );
    es.push_back( thread.start_event->get_solver_symbol() );
    es.push_back( thread.final_event->get_solver_symbol() );
  }
  dist = distinct(es);

}


//----------------------------------------------------------------------------
// build preserved programming order(ppo)
// todo: deal with double barriers that may cause bugs since they are not 
//       explicitly ordered



void wmm_event_cons::ppo_sc( const tara::thread& thread ) {

  for( auto& e : thread.events ) {
    po = po && hb_encoding.mk_ghbs( e->prev_events, e );
  }
  auto& e = thread.final_event;
  po = po && hb_encoding.mk_ghbs( e->prev_events, e );
}


bool wmm_event_cons::is_non_mem_ordered( const hb_enc::se_ptr& e1,
                                         const hb_enc::se_ptr& e2  ) {
  if( e1->is_block_head() || e2->is_block_head()  ) return false;
  if( e2->is_barrier() ||e2->is_before_barrier() ||e2->is_thread_create())
    return true;
  if( e1->is_barrier() || e1->is_after_barrier() || e1->is_thread_join() )
    return true;
  if( e2->is_thread_join() || e2->is_after_barrier() ||
      e1->is_thread_create() || e1->is_before_barrier()  ) return false;
  assert(false);
  return false;
}

bool wmm_event_cons::is_ordered_sc( const hb_enc::se_ptr& e1,
                                    const hb_enc::se_ptr& e2  ) {
  assert( e1->is_mem_op() && e2->is_mem_op() );

  return true;
}

bool wmm_event_cons::is_ordered_tso( const hb_enc::se_ptr& e1,
                                     const hb_enc::se_ptr& e2  ) {

  assert( e1->is_mem_op() && e2->is_mem_op() );
  if( e1->is_wr() && e2->is_wr() ) return true;
  if( e1->is_rd() && ( e2->is_wr() || e2->is_rd() ) ) return true;
  return false;
}

bool wmm_event_cons::is_ordered_pso( const hb_enc::se_ptr& e1,
                                     const hb_enc::se_ptr& e2  ) {
  assert( e1->is_mem_op() && e2->is_mem_op() );
  if( e1->is_wr() && e2->is_wr() && (e1->prog_v.name == e2->prog_v.name )) return true;
  if( e1->is_rd() && ( e2->is_wr() || e2->is_rd() ) ) return true;
  return false;
}




z3::expr wmm_event_cons::is_ordered_dependency( const hb_enc::se_ptr& e1,
                                                const hb_enc::se_ptr& e2  ) {

  if( !(e1->is_mem_op() && e2->is_mem_op()) ) return z3.mk_emptyexpr();

  z3::expr ret = z3.mk_emptyexpr();
  for( auto& dep : e2->data_dependency ) {
    if( dep.e == e1 ) {
      ret = dep.cond;
      break;
    }
  }
  for( auto& dep : e2->ctrl_dependency ) {
    if( dep.e == e1 ) {
      if( ret ) ret = ret || dep.cond; else ret = dep.cond;
      break;
    }
  }
  return ret;
}

bool wmm_event_cons::is_ordered_alpha( const hb_enc::se_ptr& e1,
                                       const hb_enc::se_ptr& e2  ) {
  assert( e1->is_mem_op() && e2->is_mem_op() );
  if( e1->is_wr() && e2->is_wr() && (e1->prog_v.name == e2->prog_v.name))
    return true;
  if( e1->is_rd() && ( e2->is_wr() || e2->is_rd()) &&
      (e1->prog_v.name == e2->prog_v.name)) return true;
  return false;
}

bool wmm_event_cons::check_ppo( mm_t mm,
                                const hb_enc::se_ptr& e1,
                                const hb_enc::se_ptr& e2 ) {
  if( e1->is_non_mem_op() || e2->is_non_mem_op() ) {
    return is_non_mem_ordered( e1, e2 );
  }
  // if( e1->is_barr_type() || e2->is_barr_type() ) {
  //   return is_barrier_ordered( e1, e2 );
  // }
  switch( mm ) {
  case mm_t::sc     : return is_ordered_sc   ( e1, e2 ); break;
  case mm_t::tso    : return is_ordered_tso  ( e1, e2 ); break;
  case mm_t::pso    : return is_ordered_pso  ( e1, e2 ); break;
  case mm_t::rmo    : return is_ordered_rmo  ( e1, e2 ); break;
  case mm_t::alpha  : return is_ordered_alpha( e1, e2 ); break;
  case mm_t::arm8_2 : return is_ordered_arm8_2( e1,e2 ); break;
  case mm_t::power  : return is_ordered_power( e1,e2 ); break;
  case mm_t::c11    : return is_ordered_c11( e1,e2); break;
  default:
    std::string msg = "unsupported memory model: " + string_of_mm( mm ) + "!!";
    wmm_error( msg );
  }
  return false; // dummy return
}

bool wmm_event_cons::check_ppo( const hb_enc::se_ptr& e1,
                                const hb_enc::se_ptr& e2 ) {
  return check_ppo( p.get_mm(), e1, e2 );
  return false; // dummy return
}

void wmm_event_cons::ppo_traverse( const tara::thread& thread ) {
  hb_enc::se_to_ses_map pending_map, ordered_map;
  auto& se = thread.start_event;
  pending_map[se].insert(se); // this is how it should start

  for( auto& e : thread.events ) {
    pending_map[e].insert(e);
    hb_enc::se_set tmp_pendings;
    for( auto& ep : e->prev_events )
      helpers::set_insert( tmp_pendings, pending_map[ep] );
    hb_enc::se_set& pending = pending_map[e];
    hb_enc::se_set& ordered = ordered_map[e];
    hb_enc::se_set seen_set;
    while( !tmp_pendings.empty() ) {
      auto ep = *tmp_pendings.begin();
      tmp_pendings.erase( ep );
      if( helpers::exists( seen_set, ep ) ) continue;
      seen_set.insert( ep );
      if( check_ppo( ep, e ) ) {
        po = po && hb_encoding.mk_ghbs( ep, e );
        ordered.insert( ep );
      }else{
        pending.insert( ep );
        helpers::set_insert( tmp_pendings, ordered_map[ep] );
      }
    }
  }

  auto& fe = thread.final_event;
  for( auto& e : fe->prev_events ) {
    for( auto& ep : pending_map[e] ) {
      po = po && hb_encoding.mk_ghbs( ep, fe );
    }
  }
}

void wmm_event_cons::ppo() {
  // wmm_test_ppo();

  for( unsigned t=0; t < p.size(); t++ ) {
    auto& thr = p.get_thread(t);
    auto& cr_e = p.create_map[thr.name];
    z3::expr cr_ord = p.is_mm_c11() ?
      hb_encoding.mk_hb_c11_hb( cr_e ,thr.start_event ):
      hb_encoding.mk_hbs( cr_e ,thr.start_event );
    po = po && z3::implies( cr_e->guard, thr.start_event->guard && cr_ord );
    if(       p.is_mm_sc()    ) { ppo_sc ( thr ); // sc_ppo   ( thread );
    }else if( p.is_mm_tso()   ) { ppo_traverse( thr );
    }else if( p.is_mm_pso()   ) { ppo_traverse( thr );
    }else if( p.is_mm_rmo()   ) { ppo_rmo_traverse( thr );
    }else if( p.is_mm_alpha() ) { ppo_traverse( thr );
    }else if( p.is_mm_arm8_2()) { ppo_arm8_2( thr );
    }else if( p.is_mm_power() ) { ppo_power( thr );
    }else if( p.is_mm_c11() )   { ppo_c11( thr );
    }else{                      p.unsupported_mm();
    }
    // ordering with the final events of the thread
    // and theor join points
    const auto& it = p.join_map.find( thr.name );
    if( it != p.join_map.end() ) {
      const auto& jp_pair = it->second;
      const auto join_point = jp_pair.first;
      z3::expr join_guard = jp_pair.second;
      z3::expr join_order = p.is_mm_c11() ?
        hb_encoding.mk_hb_c11_hb( thr.final_event, join_point ):
        hb_encoding.mk_hbs( thr.final_event, join_point);
      // z3::expr join_order = hb_encoding.mk_hbs( thr.final_event, join_point);
      po = po && z3::implies( thr.final_event->guard, join_guard && join_order);
    }
  }
  // po = po && dist;
  //phi_po = phi_po && barriers;
  po = po.simplify();
}


// collecting stats about the events
void wmm_event_cons::update_orderings() {
  for( unsigned i = 0; i < p.size(); i ++ ) {
    for( auto& e : p.get_thread(i).events ) {
      update_must_before( p.get_thread(i).events, e );
    }
  }

  p.update_seq_orderings();

  for( unsigned i = 0; i < p.size(); i ++ ) {
    auto rit = p.get_thread(i).events.rbegin();
    auto rend = p.get_thread(i).events.rend();
    for (; rit!= rend; ++rit)
      update_must_after( p.get_thread(i).events, *rit );
  }
  if( !p.is_mm_c11() ) {
    //todo: check if this condition is correct
    // assert(false);
    for( unsigned i = 0; i < p.size(); i ++ ) {
      for( auto& e : p.get_thread(i).events ) {
        update_ppo_before( p.get_thread(i).events, e );
      }
    }
  }

  if(0) { // todo : may after disabled for now
    for( unsigned i = 0; i < p.size(); i ++ ) {
      auto rit = p.get_thread(i).events.rbegin();
      auto rend = p.get_thread(i).events.rend();
      for (; rit!= rend; ++rit)
        update_may_after( p.get_thread(i).events, *rit );
    }
  }

  if( o.print_input > 2 ) {
    o.out() << "============================\n";
    o.out() << "must after/before relations:\n";
    o.out() << "============================\n";
    for( unsigned i = 0; i < p.size(); i ++ ) {
      hb_enc::se_vec es = p.get_thread(i).events;
      es.push_back( p.get_thread(i).start_event );
      es.push_back( p.get_thread(i).final_event );
      for( auto& e : es ) {
        o.out() << e->name() << "\nbefore: ";
        tara::debug_print( o.out(), p.must_before[e] );
        o.out() << "after: ";
        tara::debug_print( o.out(), p.must_after [e] );
        o.out() << "may after: ";
        tara::debug_print( o.out(), p.may_after [e] );
        o.out() << "ppo before: ";
        tara::debug_print( o.out(), p.ppo_before [e] );
        o.out() << "c11 release sequence heads: ";
        tara::debug_print( o.out(), p.c11_rs_heads [e] );
        o.out() << "seq dominated wr before: ";
        tara::debug_print( o.out(), p.seq_dom_wr_before [e] );
        o.out() << "seq dominated wr after: ";
        tara::debug_print( o.out(), p.seq_dom_wr_after [e] );
        o.out() << "seq before: ";
        tara::debug_print( o.out(), p.seq_before[e] );
        o.out() << "\n";
      }
    }
  }
}

void wmm_event_cons::update_must_before( const hb_enc::se_vec& es,
                                         hb_enc::se_ptr e ) {
  // hb_enc::se_tord_set to_be_visited = { e };
  hb_enc::se_to_ses_map local_ordered;
  for( auto& ep : es ) {
    if( ep->get_topological_order() > e->get_topological_order() )
      continue;
    std::vector<hb_enc::se_set> ord_sets( ep->prev_events.size() );
    unsigned i = 0;
    for( auto& epp : ep->prev_events ) {
      ord_sets[i] = local_ordered[epp];
      if( check_ppo( epp, e ) ) {
        ord_sets[i].insert( epp );
        helpers::set_insert( ord_sets[i], p.must_before[epp] );
      }
      i++;
    }
    helpers::set_intersection( ord_sets, local_ordered[ep] );
    if( e == ep ) break;
  }
  p.must_before[e] = local_ordered[e];
}


void wmm_event_cons::update_must_after( const hb_enc::se_vec& es,
                                        hb_enc::se_ptr e ) {
  hb_enc::se_to_ses_map local_ordered;
  auto rit = es.rbegin();
  auto rend = es.rend();
  for (; rit!= rend; ++rit) {
    auto& ep = *rit;
    if( ep->get_topological_order() < e->get_topological_order() )
      continue;
    std::vector<hb_enc::se_set> ord_sets( ep->post_events.size() );
    unsigned i = 0;
    for( auto it = ep->post_events.begin(); it != ep->post_events.end(); it++) {
      const hb_enc::depends& dep = *it;
      hb_enc::se_ptr epp = dep.e;
      ord_sets[i] = local_ordered[epp];
      if( check_ppo( e, epp ) ) {
        ord_sets[i].insert( epp );
        helpers::set_insert( ord_sets[i], p.must_after[epp] );
      }
      i++;
    }
    helpers::set_intersection( ord_sets, local_ordered[ep] );
    if( e == ep ) break;
  }
  p.must_after[e] = local_ordered[e];
}

void wmm_event_cons::pointwise_and( const hb_enc::depends_set& dep_in,
                                    z3::expr cond,
                                    hb_enc::depends_set& dep_out ) {
  for( const hb_enc::depends& d : dep_in ) {
    z3::expr c = d.cond && cond;
    c = c.simplify();
    dep_out.insert( hb_enc::depends( d.e, c ));
  }
}

void wmm_event_cons::update_may_after( const hb_enc::se_vec& es,
                                       hb_enc::se_ptr e ) {
  hb_enc::se_to_depends_map local_ordered;
  auto rit = es.rbegin();
  auto rend = es.rend();
  for (; rit!= rend; ++rit) {
    auto ep = *rit;
    if( ep->get_topological_order() < e->get_topological_order() )
      continue;
    std::vector<hb_enc::depends_set> temp_vector;
    // unsigned i = 0;
    for( const hb_enc::depends& dep : ep->post_events ){
      const hb_enc::se_ptr epp = dep.e;
      hb_enc::depends_set temp;
      z3::expr epp_cond = z3.mk_false();
      // std::cout << e->name() << " "<< ep->name() << " "<< epp->name();
      // std::cout << "\n";
      // tara::debug_print( std::cout, p.may_after[epp] );
      // tara::debug_print( std::cout, local_ordered[epp] );
      //todo: rmo support is to be added
      //      check_ppo does the half work for rmo
      if( check_ppo( e, epp ) ) {
        hb_enc::depends_set temp3;
        hb_enc::join_depends_set( p.may_after[epp], local_ordered[epp], temp3 );
        pointwise_and( temp3, dep.cond, temp );
        epp_cond = dep.cond;
      } else {
     	pointwise_and( local_ordered[epp], dep.cond, temp );
      }
      temp.insert( hb_enc::depends( epp, epp_cond ) );
      temp_vector.push_back( temp );
      // i++;
    }
    hb_enc::join_depends_set( temp_vector, local_ordered[ep] );
    if( e == ep ) break;
  }
  p.may_after[e] = local_ordered[e];
}

void wmm_event_cons::update_ppo_before( const hb_enc::se_vec& es,
                                       hb_enc::se_ptr e ) {
  hb_enc::se_to_depends_map local_ordered;
  for( auto& ep : es ) {
    if( ep->get_topological_order() > e->get_topological_order() ) continue;
    std::vector<hb_enc::depends_set> ord_sets( ep->prev_events.size() );
    unsigned i = 0;
    for( auto& epp : ep->prev_events ) {
      ord_sets[i] = local_ordered[epp];
      if( check_ppo( epp, e ) ) {
        if( epp->is_mem_op() )
                hb_enc::join_depends_set(  epp, z3.mk_true(), ord_sets[i] );
        hb_enc::join_depends_set(   p.ppo_before[epp], ord_sets[i] );
      }else{
        if( epp->is_mem_op() )
          hb_enc::join_depends_set( epp, z3.mk_false(), ord_sets[i] );
      }
      i++;
    }
    hb_enc::meet_depends_set( ord_sets, local_ordered[ep] );
    if( e == ep ) break;
  }
  p.ppo_before[e] = local_ordered[e];
}

void wmm_event_cons::run() {
  update_orderings();

  ppo(); // build hb formulas to encode the preserved program order
  if( p.is_mm_c11() ) {
    ses_c11();
  }else{
    distinct_events();
    ses();
  }

  if ( o.print_phi ) {
    o.out() << "(" << endl
            << "phi_po : \n" << (po && dist) << endl
            << "wf     : \n" << wf           << endl
            << "rf     : \n" << rf           << endl
            << "grf    : \n" << grf          << endl
            << "fr     : \n" << fr           << endl
            << "ws     : \n" << ws           << endl
            << "thin   : \n" << thin         << endl
            << "phi_prp: \n" << p.phi_prp    << endl
            << ")" << endl;
  }

  p.phi_po  = po && dist;
  p.phi_ses = wf && grf && fr && ws ;
  if( !p.is_mm_arm8_2() ) {
    p.phi_ses = p.phi_ses && thin; //todo : undo this update
  }
}


