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
 */

#include "program.h"
#include "wmm.h"
#include "cssa_exception.h"
#include "helpers/helpers.h"
// #include "helpers/int_to_string.h"
#include <string.h>
#include "helpers/z3interf.h"
#include <utility>
#include <assert.h>
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

//----------------------------------------------------------------------------
// New code
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// hb utilities

// ghb = guarded hb
// z3::expr wmm_event_cons::wmm_mk_ghb( const hb_enc::se_ptr& before,
//                                      const hb_enc::se_ptr& after ) {
//   return implies( before->guard && after->guard,
//                   hb_encoding.make_hbs( before, after ) );
// }

// // thin air ghb
// z3::expr wmm_event_cons::wmm_mk_ghb_thin( const hb_enc::se_ptr& before,
// 				   const hb_enc::se_ptr& after ) {
//   return implies( before->guard && after->guard,
//                   hb_encoding.make_hb_thin( before, after ) );
// }

//----------------------------------------------------------------------------
// memory model utilities


bool wmm_event_cons::in_grf( const hb_enc::se_ptr& wr,
                             const hb_enc::se_ptr& rd ) {
  if( p.is_mm_sc() ) {
    return true;
  }else if( p.is_mm_tso() || p.is_mm_pso() ||
            p.is_mm_rmo() || p.is_mm_alpha() ) {
    return wr->tid != rd->tid; //only events with diff threads are part of grf
  }else{
    p.unsupported_mm();
    return false;
  }
}


//----------------------------------------------------------------------------
// coherence preserving code
// po-loc must be compatible with
// n - nothing to do
// r - relaxed
// u - not visible
// (pp-loc)/(ses)
//                     sc   tso   pso   rmo
//  - ws      (w->w)   n/n  n/n   n/n   n/n
//  - fr      (r->w)   n/n  r/n   r/n   r/n
//  - fr;rf   (r->r)   n/n  n/u   n/u   r/u
//  - rf      (w->r)   n/n  n/u   n/u   n/u
//  - ws;rf   (w->r)   n/n  n/u   n/u   n/u

bool wmm_event_cons::anti_ppo_read( const hb_enc::se_ptr& wr,
                                    const hb_enc::se_ptr& rd ) {
  // preventing coherence violation - rf
  // (if rf is local then may not visible to global ordering)
  assert( wr->tid == p.no_of_threads() ||
          rd->tid == p.no_of_threads() ||
          wr->prog_v.name == rd->prog_v.name );
  if( p.is_mm_sc() || p.is_mm_tso() || p.is_mm_pso()
      || p.is_mm_rmo() || p.is_mm_alpha() ) {
    // should come here for those memory models where rd-wr on
    // same variables are in ppo
    if( is_po( rd, wr ) ) {
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
  assert( wr->tid == p.no_of_threads() || rd->tid == p.no_of_threads() ||
          wr->prog_v.name == rd->prog_v.name );
  if( p.is_mm_sc() || p.is_mm_tso() || p.is_mm_pso() || p.is_mm_rmo() || p.is_mm_alpha()) {
    if( is_po( wr, rd ) ) {
      return true;
    }
    //
  }else{
    p.unsupported_mm();
  }
  return false;
}

bool wmm_event_cons::is_rd_rd_coherance_preserved() {
  if( p.is_mm_sc() || p.is_mm_tso() || p.is_mm_pso() || p.is_mm_alpha()) {
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

z3::expr wmm_event_cons::get_rf_bvar( const variable& v1,
                                      hb_enc::se_ptr wr, hb_enc::se_ptr rd,
                                      bool record ) {
  std::string bname = v1+"-"+wr->name()+"-"+rd->name();
  z3::expr b = z3.c.bool_const(  bname.c_str() );
  if( record ) p.reading_map.insert( std::make_tuple(bname,wr,rd) );
  return b;
}

void wmm_event_cons::ses() {
  // For each global variable we need to construct
  // - wf  well formed
  // - rf  read from
  // - grf global read from
  // - fr  from read
  // - ws  write serialization

  // z3::context& c = _z3.c;

  for( const variable& v1 : p.globals ) {
    const auto& rds = p.rd_events[v1];
    const auto& wrs = p.wr_events[v1];
    unsigned c_tid = 0;
    hb_enc::se_set tid_rds;
    for( const hb_enc::se_ptr& rd : rds ) {
      z3::expr some_rfs = z3.c.bool_val(false);
      z3::expr rd_v = rd->get_var_expr(v1);
      if( rd->tid != c_tid ) {
        tid_rds.clear();
        c_tid = rd->tid;
      }
      for( const hb_enc::se_ptr& wr : wrs ) {
        if( anti_ppo_read( wr, rd ) ) continue;
        z3::expr wr_v = wr->get_var_expr( v1 );
        z3::expr b = get_rf_bvar( v1, wr, rd );
        some_rfs = some_rfs || b;
        z3::expr eq = ( rd_v == wr_v );
        // well formed
        wf = wf && implies( b, wr->guard && eq);
        // read from
        z3::expr new_rf = implies( b, hb_encoding.mk_ghbs( wr, rd ) );
        rf = rf && new_rf;
        z3::expr new_thin = implies( b, hb_encoding.mk_ghb_thin( wr, rd ) );
        thin = thin && new_thin;
        //global read from
        if( in_grf( wr, rd ) ) grf = grf && new_rf;
        // from read
        for( const hb_enc::se_ptr& after_wr : wrs ) {
          if( after_wr->name() != wr->name() ) {
            auto cond = b && hb_encoding.mk_ghbs(wr,after_wr) && after_wr->guard;
            if( anti_po_loc_fr( rd, after_wr ) ) {
              fr = fr && !cond;
            }else{
              auto new_fr = hb_encoding.mk_ghbs( rd, after_wr );
              if( is_rd_rd_coherance_preserved() ) {
                for( auto before_rd : tid_rds ) {
                  //disable this code for rmo
                  z3::expr anti_coherent_b =
                    get_rf_bvar( v1, after_wr, before_rd, false );
                  new_fr = !anti_coherent_b && new_fr;
                }
              }
              fr = fr && implies( cond , new_fr );
            }
          }
        }
      }
      wf = wf && implies( rd->guard, some_rfs );
      tid_rds.insert( rd );
    }

    // write serialization
    // todo: what about ws;rf
    auto it1 = wrs.begin();
    for( ; it1 != wrs.end() ; it1++ ) {
      const hb_enc::se_ptr& wr1 = *it1;
      auto it2 = it1;
      it2++;
      for( ; it2 != wrs.end() ; it2++ ) {
        const hb_enc::se_ptr& wr2 = *it2;
        if( wr1->tid != wr2->tid && // Why this condition?
            !wr1->is_pre() && !wr2->is_pre() // no initializations
            ) {
          ws = ws && ( hb_encoding.mk_ghbs( wr1, wr2 ) ||
                       hb_encoding.mk_ghbs( wr2, wr1 ) );
        }
      }
    }

    // dependency
    for( const hb_enc::se_ptr& wr : wrs )
      for( auto& rd : p.data_dependency[wr] )
        //todo : should the following be guarded??
          thin = thin && hb_encoding.mk_hb_thin( rd.e, wr );

  }
}


//----------------------------------------------------------------------------
// declare all events happen at different time points

void wmm_event_cons::distinct_events() {

  z3::expr_vector loc_vars( z3.c );

  for( auto& e : p.init_loc ) {
    loc_vars.push_back( e->get_solver_symbol() );
  }

  for( auto& e : p.post_loc ) {
    loc_vars.push_back( e->get_solver_symbol() );
  }

  for( unsigned t=0; t < p.no_of_threads(); t++ ) {
    auto& thread = p.get_thread(t);
    assert( thread.size() > 0 );
    // for(auto& e : thread.events )
    //   loc_vars.push_back( e->get_solver_symbol() );
    for( unsigned j=0; j < thread.size(); j++ ) {
      for( auto& rd : thread[j].rds ) {
        loc_vars.push_back( rd->get_solver_symbol() );
      }
      for( auto& wr : thread[j].wrs ) {
        loc_vars.push_back( wr->get_solver_symbol() );
      }
    }
    loc_vars.push_back( thread.start_event->get_solver_symbol() );
    loc_vars.push_back( thread.final_event->get_solver_symbol() );
  }
  dist = distinct(loc_vars);

}


//----------------------------------------------------------------------------
// build preserved programming order(ppo)
// todo: deal with double barriers that may cause bugs since they are not 
//       explicitly ordered

void wmm_event_cons::new_ppo_sc( const tara::thread& thread ) {

  for( auto& e : thread.events ) {
    po = po && hb_encoding.mk_ghbs( e->prev_events, e );
  }
  auto& e = thread.final_event;
  po = po && hb_encoding.mk_ghbs( e->prev_events, e );
}


bool wmm_event_cons::is_barrier_ordered( const hb_enc::se_ptr e1,
                                         const hb_enc::se_ptr e2  ) {
  if( e2->is_barrier() || e2->is_before_barrier() ) return true;
  if( e1->is_barrier() || e1->is_after_barrier()  ) return true;
  if( e2->is_after_barrier() || e1->is_before_barrier()  ) return false;
  assert(false);
  return false;
}

bool wmm_event_cons::is_ordered_tso( const hb_enc::se_ptr e1,
                                     const hb_enc::se_ptr e2  ) {
  if( e1->is_barr_type() || e2->is_barr_type() ) {
    return is_barrier_ordered( e1, e2 );
  }

  assert( e1->is_mem_op() && e2->is_mem_op() );
  if( e1->is_wr() && e2->is_wr() ) return true;
  if( e1->is_rd() && ( e2->is_wr() || e2->is_rd() ) ) return true;
  return false;
}

void wmm_event_cons::new_ppo_tso( const tara::thread& thread ) {
  hb_enc::se_to_ses_map pending_map;
  auto& se = thread.start_event;
  pending_map[se].insert(se); // this is how it should start

  for( auto& e : thread.events ) {
    pending_map[e].insert(e);
    hb_enc::se_set pendings;
    for( auto& ep : e->prev_events ) {
      hb_enc::se_set& to_be_ordered = pending_map[ep];
      pendings.insert( to_be_ordered.begin(), to_be_ordered.end() );
    }
    hb_enc::se_set& not_ordered = pending_map[e];
    for( auto& ep : pendings ) {
      if( is_ordered_tso( ep, e ) ) {
        po = po && hb_encoding.mk_ghbs( ep, e );
      }else{
        not_ordered.insert( ep );
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

void wmm_event_cons::sc_ppo( const tara::thread& thread ) {
  unsigned tsize = thread.size();
  hb_enc::se_set last = p.init_loc;

  for( unsigned j = 0; j < tsize; j++ ) {
    auto& rds = thread[j].rds; auto& wrs = thread[j].wrs;
    if( rds.empty() && wrs.empty() ) continue;
    auto& after = rds.empty() ? wrs : rds;
    po = po &&
      hb_encoding.mk_hbs( last, after ) && hb_encoding.mk_hbs( rds, wrs );
    last = wrs.empty() ? rds : wrs;
  }
  po = po && hb_encoding.mk_hbs( last, p.post_loc);
}

void wmm_event_cons::tso_ppo( const tara::thread& thread ) {
  hb_enc::se_set last_rds = p.init_loc, last_wrs = p.init_loc;
  bool rd_occured = false;
  hb_enc::se_set barr_events = p.init_loc;
  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier( thread[j].type ) ) {
      po = po && hb_encoding.mk_hbs( last_rds, thread[j].barr );
      po = po && hb_encoding.mk_hbs( last_wrs, thread[j].barr );
      last_rds = last_wrs = thread[j].barr;
      rd_occured = false;
    }else{
      // if(!is_barrier( thread[j].type )){
      auto& rds = thread[j].rds; auto& wrs = thread[j].wrs;
      if( !rds.empty() ) {
        po = po && hb_encoding.mk_hbs( last_rds, rds );
        last_rds = rds;
        rd_occured = true;
      }
      if( !wrs.empty() ) {
        po = po && hb_encoding.mk_hbs( last_wrs, wrs );
        last_wrs = wrs;

        if( rd_occured ) {
          po = po && hb_encoding.mk_hbs(last_rds, wrs);
        }
      }
    }
  }
  po = po && hb_encoding.mk_hbs(last_wrs, p.post_loc);
}

void wmm_event_cons::pso_ppo( const tara::thread& thread ) {
  hb_enc::var_to_se_map last_wr;
  assert( p.init_loc.size() == 1);
  hb_enc::se_ptr init_l = *p.init_loc.begin();
  for( auto& g : p.globals ) last_wr[g] = init_l;

  bool no_rd_occurred = true;
  hb_enc::se_set last_rds = p.init_loc;
  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier(thread[j].type) ) {
      po = po && hb_encoding.mk_hbs( last_rds, thread[j].barr );
      last_rds = thread[j].barr;
      no_rd_occurred = true;
      assert( thread[j].barr.size() == 1 );
      hb_enc::se_ptr barr = *thread[j].barr.begin();
      for( auto& g : p.globals ) {
        po = po && hb_encoding.mk_hbs( last_wr[g], barr );
        last_wr[g] = barr;
      }
    }else{
      auto& rds = thread[j].rds;
      auto& wrs = thread[j].wrs;
      if( rds.size() > 0 ) {
        po = po && hb_encoding.mk_hbs( last_rds, rds );
        last_rds = rds;
        no_rd_occurred = false;
      }

      for( auto wr : wrs ) {
        const variable& v = wr->prog_v;
        po = po && hb_encoding.mk_hbs( last_wr[v], wr );
        last_wr[v] = wr;
        if( !no_rd_occurred ) {
          po = po && hb_encoding.mk_hbs( last_rds, wr );
        }
      }
    }
  }
  // only writes must be ordered with post event
  for( auto& g : p.globals ) {
    po = po && hb_encoding.mk_hbs( last_wr[g], p.post_loc );
  }
  //po = po && barriers;
}

void wmm_event_cons::rmo_ppo( const tara::thread& thread ) {
  hb_enc::var_to_se_map last_rd, last_wr;
  hb_enc::se_set collected_rds;

  assert( p.init_loc.size() == 1);
  hb_enc::se_ptr barr = *p.init_loc.begin();
  for( auto& g : p.globals ) last_rd[g] = last_wr[g] = barr;

  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier( thread[j].type ) ) {
      assert( thread[j].barr.size() == 1);
      barr = *thread[j].barr.begin();
      for( hb_enc::se_ptr rd : collected_rds) {
        po = po && hb_encoding.mk_hbs( rd, barr );
      }
      collected_rds.clear();

      for( auto& g : p.globals ) {
        po = po && hb_encoding.mk_hbs( last_wr[g], barr );
        last_rd[g] = last_wr[g] = barr;
      }
    }else{
      for( auto rd : thread[j].rds ) {
        po = po && hb_encoding.mk_hbs( barr, rd );
        last_rd[rd->prog_v]  = rd;
        collected_rds.insert( rd );
	for( auto& read : p.ctrl_dependency[rd]) //todo: support condition
            po = po && hb_encoding.mk_hbs( read.e, rd );
      }

      for( auto wr : thread[j].wrs ) {
        variable v = wr->prog_v;

        po = po && hb_encoding.mk_hbs( last_wr[v], wr );
        po = po && hb_encoding.mk_hbs( last_rd[v], wr );
        collected_rds.erase( last_rd[v] );// optimization??

        for( auto& rd : p.data_dependency[wr] )
          po = po && hb_encoding.mk_hbs( rd.e, wr ); //todo: support cond
	for( auto& rd : p.ctrl_dependency[wr])
            po = po && hb_encoding.mk_hbs( rd.e, wr ); //todo: support cond

        last_wr[v] = wr;
      }
    }
  }

  for( auto& g : p.globals )
    po = po && hb_encoding.mk_hbs( last_wr[g], p.post_loc );
}

void wmm_event_cons::alpha_ppo( const tara::thread& thread ) {
  hb_enc::var_to_se_map last_wr, last_rd;
  assert( p.init_loc.size() == 1);
  hb_enc::se_ptr barr = *p.init_loc.begin();
  for( auto& g : p.globals ) last_rd[g] = last_wr[g] = barr;

  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier( thread[j].type ) ) {
      assert( thread[j].barr.size() == 1);
      barr = *thread[j].barr.begin();
      for( auto& g : p.globals ) {
        po = po && hb_encoding.mk_hbs( last_rd[g], barr );
        po = po && hb_encoding.mk_hbs( last_wr[g], barr );
        last_rd[g] = last_wr[g] = barr;
      }
    }else{
      for( auto rd : thread[j].rds ) {
        const variable& v = rd->prog_v;
        po = po && hb_encoding.mk_hbs( last_rd[v], rd ); //read-read to same loc
        last_rd[v] = rd;
      }
      for( auto wr : thread[j].wrs ){
        const variable& v = wr->prog_v;
        po = po && hb_encoding.mk_hbs( last_wr[v], wr ); //write-write to same loc
        po = po && hb_encoding.mk_hbs( last_rd[v], wr ); //read-write to same loc
        last_wr[v] = wr;
      }
    }
  }
  for( auto& g : p.globals )
    po = po && hb_encoding.mk_hbs( last_wr[g], p.post_loc );
}

void wmm_event_cons::power_ppo( const tara::thread& thread ) {
  p.unsupported_mm();
}

void wmm_event_cons::ppo() {
  // wmm_test_ppo();

  for( unsigned t=0; t < p.no_of_threads(); t++ ) {
    auto& thr = p.get_thread(t);
    po = po && hb_encoding.mk_ghbs( p.create_map[thr.name],thr.start_event );
    if(       p.is_mm_sc()    ) { new_ppo_sc( thr ); // sc_ppo   ( thread );
    }else if( p.is_mm_tso()   ) { new_ppo_tso( thr ); //tso_ppo  ( thr );
    }else if( p.is_mm_pso()   ) { pso_ppo  ( thr );
    }else if( p.is_mm_rmo()   ) { rmo_ppo  ( thr );
    }else if( p.is_mm_alpha() ) { alpha_ppo( thr );
    }else if( p.is_mm_power() ) { power_ppo( thr );
    }else{                      p.unsupported_mm();
    }
    if( hb_enc::se_ptr& join_point = p.join_map[thr.name] )
      po = po && hb_encoding.mk_ghbs( thr.final_event, join_point );
  }
  // po = po && dist;
  //phi_po = phi_po && barriers;
}

void wmm_event_cons::wmm_test_ppo() {
  unsigned t;
  po = z3.mk_true();
  for(t=0;t<p.no_of_threads();t++){sc_ppo(p.get_thread(t));}
  std::cerr << "\nsc" << po; po = z3.mk_true();
  for(t=0;t<p.no_of_threads();t++){tso_ppo(p.get_thread(t));}
  std::cerr << "\ntso" << po; po = z3.mk_true();
  for(t=0;t<p.no_of_threads();t++){pso_ppo(p.get_thread(t));}
  std::cerr << "\npso" << po; po = z3.mk_true();
  for(t=0;t<p.no_of_threads();t++){rmo_ppo(p.get_thread(t));}
  std::cerr << "\nrmo" << po; po = z3.mk_true();
}


//----------------------------------------------------------------------------

z3::expr wmm_event_cons::insert_tso_barrier( const tara::thread & thread,
                                             unsigned instr,
                                             hb_enc::se_ptr new_barr ) {
  z3::expr hbs = z3.mk_true();

  bool before_found = false;
  unsigned j = instr;
  while( j != 0 )  {
    j--;
    if( !thread[j].wrs.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].wrs, new_barr );
      before_found = true;
      break;
    }
    if( !before_found && !thread[j].rds.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].rds, new_barr );
      before_found = true;
    }
  }

  bool after_found = false;
  for(j = instr; j < thread.size(); j++ ) {
    if( !thread[j].rds.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].rds, new_barr );
      after_found = true;
      break;
    }
    if( !after_found && !thread[j].wrs.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].wrs, new_barr );
      after_found = true;
    }
  }
  if( before_found && after_found )  return hbs;
  return z3.mk_true();
}

z3::expr wmm_event_cons::insert_pso_barrier( const tara::thread & thread,
                                             unsigned instr,
                                             hb_enc::se_ptr new_barr ) {
  //todo stop at barriers
  z3::expr hbs = z3.mk_true();

  variable_set found_wrs = p.globals;
  bool before_found = false;
  unsigned j = instr;
  while( j != 0 )  {
    j--;
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && hb_encoding.mk_hbs( wr, new_barr );
        found_wrs.erase( it );
        before_found = true;
      }
    }
    if( found_wrs.empty() ) break;
    if( !before_found && !thread[j].rds.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].rds, new_barr );
      before_found = true;
    }
  }

  found_wrs = p.globals;
  bool after_found = false;
  for( j = instr; j < thread.size(); j++ )  {
    if( !thread[j].rds.empty() ) {
      hbs = hbs && hb_encoding.mk_hbs( thread[j].rds, new_barr );
      after_found = true;
      break;
    }
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && hb_encoding.mk_hbs( wr, new_barr );
        found_wrs.erase( it );
        after_found = true;
      }
    }
  }

  if( before_found && after_found )  return hbs;
  return z3.mk_true();
}

z3::expr wmm_event_cons::insert_rmo_barrier( const tara::thread & thread,
                                             unsigned instr,
                                             hb_enc::se_ptr new_barr ) {
  z3::expr hbs = z3.mk_true();

  bool before_found = false;
  unsigned j = instr;
  hb_enc::se_set collected_rds;
  variable_set found_wrs = p.globals;
  while( j != 0 )  {
    j--;
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && hb_encoding.mk_hbs( wr, new_barr );
        found_wrs.erase( it );
        before_found = true;
      }
      for( auto rd : p.data_dependency[wr] ) {
        collected_rds.insert( rd.e );
      }
    }

    for( auto rd: thread[j].rds ) {
      auto it = collected_rds.find(rd);
      if( it != collected_rds.end() ) {
        collected_rds.erase( it );
      }else{
        hbs = hbs && hb_encoding.mk_hbs( rd, new_barr );
        before_found = true;
      }
    }
  }

  bool after_found = false;
  found_wrs = p.globals;
  for( unsigned j = instr; j < thread.size(); j++ ) {

    for( auto rd : thread[j].rds ) {
      const variable& v = rd->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && hb_encoding.mk_hbs( new_barr, rd );
        after_found = true;
        found_wrs.erase( it );
      }
    }

    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end()
          // && data_dependency[wr].empty() // todo: optimization
          ) {
        hbs = hbs && hb_encoding.mk_hbs( new_barr, wr );
        found_wrs.erase( it );
        after_found = true;
      }
    }
    if( found_wrs.empty() ) break;
  }

  if( before_found && after_found )  return hbs;
  return z3.mk_true();
}

z3::expr wmm_event_cons::insert_barrier(unsigned tid, unsigned instr) {
  auto & thr = p.get_thread(tid);//*threads[instr];
  assert( instr < thr.size() );

  //todo : prepage contraints for barrier
  hb_enc::se_set last;
  hb_enc::se_ptr barr =
    mk_se_ptr_old( hb_encoding, tid, instr, thr[instr].loc->name,
                   hb_enc::event_t::barr, last );
  p.add_event( tid, barr );assert(false); // todo: wrong insertion and
                                          //        needs update
  z3::expr ord(z3.c);
  if(      p.is_mm_tso()) { ord = insert_tso_barrier( thr, instr, barr );
  }else if(p.is_mm_pso()) { ord = insert_pso_barrier( thr, instr, barr );
  }else if(p.is_mm_rmo()) { ord = insert_rmo_barrier( thr, instr, barr );
  }else{                  p.unsupported_mm();
  }
  return ord;
}


wmm_event_cons::wmm_event_cons( helpers::z3interf& _z3,
                                api::options& _o,
                                hb_enc::encoding& _hb_encoding,
                                tara::program& _p )
: z3( _z3 )
,o( _o )
,hb_encoding( _hb_encoding )
,p( _p ) {
}

void wmm_event_cons::run() {
  distinct_events(); // Rd/Wr events on globals are distinct
  ppo(); // build hb formulas to encode the preserved program order
  ses(); // build symbolic event structure
  // barrier(); // build barrier// ppo already has the code

  if ( o.print_phi ) {
    o.out() << "(" << endl
            << "phi_po : \n" << (po && dist) << endl
            << "wf     : \n" << wf           << endl
            << "rf     : \n" << rf           << endl
            << "grf    : \n" << grf          << endl
            << "fr     : \n" << fr           << endl
            << "ws     : \n" << ws           << endl
            << "thin   : \n" << thin         << endl
            << ")" << endl;
  }

  p.phi_po = po && dist;
  p.phi_ses = wf && grf && fr && ws && thin;
}


//----------------------------------------------------------------------------
// unused code

// void program::wmm_add_barrier(int tid, int instr) {
//   //assert((threads[tid]->instructions[instr]->type == instruction_type::fence) || (threads[tid]->instructions[instr]->type == instruction_type::barrier));
//   thread & thread= *threads[tid];
//   auto& barrier_before = thread[instr+1].rds; // everything before (above) is ordered wrt this barrier
//   auto& barrier_after = thread[instr].wrs;   // everything after (below) is ordered wrt this barrier
//   auto& rds_before = thread[instr].rds;
//   auto& rds_after = thread[instr+1].rds;
//   auto& wrs_before = thread[instr].wrs;
//   auto& wrs_after = thread[instr+1].wrs;

//   if( !rds_before.empty() || !wrs_before.empty() ) {
//     if( !rds_before.empty() && !wrs_before.empty() ) {
//           phi_po = phi_po && _hb_encoding.make_hbs(rds_before , barrier_before);
//           phi_po = phi_po && _hb_encoding.make_hbs(wrs_before , barrier_before);
//     }else if( !rds_before.empty() ) {
//       phi_po = phi_po && _hb_encoding.make_hbs( rds_before , barrier_before );
//     } else if( !wrs_before.empty() ) {
//       phi_po = phi_po && _hb_encoding.make_hbs( wrs_before , barrier_before );
//     }

//     for(int j=instr-1; j>=0; j-- ) {
//       rds_before = thread[j].rds;
//       wrs_before = thread[j].wrs;
//       if(!rds_before.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( rds_before , barrier_before );
//       }
//       if(!wrs_before.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( wrs_before , barrier_before );
//       }
//     }
//   }

//   if( !rds_after.empty() || !wrs_after.empty() ) {
//     if( !rds_after.empty() && !wrs_after.empty() ) {
//       phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , rds_after );
//       phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , wrs_after );
//     } else if( !rds_after.empty() ) {
//       phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , rds_after );
//     } else if( !wrs_after.empty() ) {
//       phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , wrs_after );
//     }

//     for(unsigned i=instr+2; i<thread.size(); i++ ) {
//       rds_after = thread[i].rds;
//       wrs_after = thread[i].wrs;
//       if(!rds_after.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , rds_after );
//       }
//       if(!wrs_after.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( barrier_after , wrs_after );
//       }
//     }
//   }
// }

// void program::barrier() {
//   //return;
//   int first_read;
//   for( auto it1=tid_to_instr.begin();it1!=tid_to_instr.end();it1++) {
//     first_read=0;
//     thread& thread=*threads[it1->first];
//     auto& barrier = thread[it1->second].barr;
//     auto& rds_before = thread[it1->second-1].rds;
//     auto& rds_after = thread[it1->second+1].rds;
//     auto& wrs_before = thread[it1->second-1].wrs;
//     auto& wrs_after = thread[it1->second+1].wrs;

//     if(is_mm_tso()) {
//       if( !wrs_before.empty() ) {
//         phi_po = phi_po && _hb_encoding.make_hbs( wrs_before, barrier );
//       } else if(!rds_before.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( rds_before, barrier );
//         for(int j=it1->second-2; j>=0; j-- ) {
//           wrs_before = thread[j].wrs;
//           if(!wrs_before.empty()) {
//             phi_po = phi_po && _hb_encoding.make_hbs( wrs_before, barrier );
//             break;
//           }
//         }
//       }
//       if( !rds_after.empty() ) {
//         phi_po = phi_po && _hb_encoding.make_hbs(barrier, rds_after );
//       }
//     } else if(!wrs_after.empty()) {
//       phi_po = phi_po && _hb_encoding.make_hbs( barrier, wrs_after );

//       for( unsigned j=it1->second+2; j!=thread.size(); j++ ) {
//         rds_after = thread[j].rds;
//         if(!wrs_before.empty()) {
//           phi_po = phi_po && _hb_encoding.make_hbs( barrier, rds_after );
//           break;
//         }
//       }
//     }
//     else if(is_mm_pso()) {
//       if( !wrs_before.empty() ) {
//         phi_po = phi_po && _hb_encoding.make_hbs( wrs_before, barrier);
//       }
//       else if(!rds_before.empty()) {
//         phi_po = phi_po && _hb_encoding.make_hbs( rds_before, barrier );
//         for(int j=it1->second-2; j>=0; j-- ){
//           wrs_before = thread[j].wrs;
//           if(!wrs_before.empty()){
//             phi_po = phi_po && _hb_encoding.make_hbs( wrs_before, barrier );

//           }
//         }
//       }
//       if(!wrs_after.empty() || !rds_after.empty()){
//         if( !rds_after.empty() && first_read==0 ){
//           phi_po = phi_po && _hb_encoding.make_hbs( barrier, rds_after );
//           first_read=1;
//         }
//         if(!wrs_after.empty()){
//           phi_po = phi_po && _hb_encoding.make_hbs( barrier, wrs_after );
//         }
//         for( unsigned i=it1->second+2; i<=thread.size()-1; i++ ){
//           rds_after = thread[i].rds;
//           wrs_after = thread[i].wrs;
//           if(!rds_after.empty() && first_read==0){
//             phi_po = phi_po && _hb_encoding.make_hbs( barrier, rds_after );
//             first_read=1;
//           }
//           if(!wrs_after.empty()) {
//             phi_po = phi_po && _hb_encoding.make_hbs( barrier, wrs_after );
//           }
//         }
//       }
//     }
//   }
// }

//----------------------------------------------------------------------------
