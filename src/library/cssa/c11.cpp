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

//*****************
// todo
//   --- support race checking
//   --- support RA fences
//   --- ????

bool wmm_event_cons::is_ordered_c11( const hb_enc::se_ptr& e1,
                                     const hb_enc::se_ptr& e2  ) {
  //todo : c11, formalize what is must before.
  return false;
}

void wmm_event_cons::ppo_c11( const tara::thread& thread ) {

  for( auto& e : thread.events ) {
    po = po && hb_encoding.mk_ghbs_c11_hb( e->prev_events, e );
  }
  auto& e = thread.final_event;
  po = po && hb_encoding.mk_ghbs_c11_hb( e->prev_events, e );
}

z3::expr wmm_event_cons::get_rel_seq_bvar( hb_enc::se_ptr wrp,
                                           hb_enc::se_ptr wr,
                                           bool record ) {
  std::string bname = "res-seq-"+wrp->name()+"-"+wr->name();
  z3::expr b = z3.c.bool_const(  bname.c_str() );
  // todo: do need to record the interupption bit
  if( record )
    p.rel_seq_map[wr].insert( std::make_pair( bname, wrp ) );
  return b;
}

void wmm_event_cons::distinct_events_c11() {

  z3::expr_vector es( z3.c );

  es.push_back( p.init_loc->get_c11_hb_solver_symbol() );

  if( p.post_loc )
    es.push_back( p.post_loc->get_c11_hb_solver_symbol() );

  for( unsigned t=0; t < p.size(); t++ ) {
    auto& thread = p.get_thread(t);
    for(auto& e : thread.events )
      es.push_back( e->get_c11_hb_solver_symbol() );
    es.push_back( thread.start_event->get_c11_hb_solver_symbol() );
    es.push_back( thread.final_event->get_c11_hb_solver_symbol() );
  }
  dist = distinct(es);

}

void wmm_event_cons::ses_c11() {
  distinct_events_c11();

  // For each global variable we need to construct
  // - wf  well formed
  // - rf  read from
  // - grf global read from
  // - fr  from read
  // - ws  write serialization

  // //todo: disable the following code if rd rd coherence not preserved
  // hb_enc::se_to_ses_map prev_rds;
  // for( unsigned t = 0; t < p.size(); t++ ) {
  //   // const auto& thr = p.get_thread( t );
  //   for( auto e : p.get_thread( t ).events ) {
  //     hb_enc::se_set& rds = prev_rds[e];
  //     for( auto ep : e->prev_events ) {
  //       rds.insert( prev_rds[ep].begin(), prev_rds[ep].end() );
  //       if( ep->is_rd() ) rds.insert( ep );
  //     }
  //   }
  // }

  for( const auto& v1 : p.globals ) {
    const auto& rds = p.rd_events[v1];
    const auto& wrs = p.wr_events[v1];
    for( const hb_enc::se_ptr& wr : wrs ) {
      if( wr->is_at_least_rls() ) continue;
      hb_enc::depends_set& deps = p.c11_rs_heads[wr];
      for( auto& dep : deps ) { // empty set if wr is a release
        z3::expr no_interup = z3.mk_true();
        for( auto& wrpp : wrs ) {
          if( wrpp->tid != wr->tid && !wrpp->is_update() && wrpp->is_wr() ) {
            no_interup = no_interup &&
              ( hb_encoding.mk_ghb_c11_mo( wrpp, dep.e ) ||
                hb_encoding.mk_ghb_c11_mo( wr  , wrpp  ) );
          }
        }
        z3::expr b = get_rel_seq_bvar( dep.e, wr );
        rel_seq = rel_seq && implies( wr->guard && dep.cond && no_interup, b );
      }
      if( wr->is_update() ) {
        for( auto& wrp : wrs ) {
          if( wrp->is_at_least_rls() && wrp->tid != wr->tid ) {
            z3::expr no_between = z3.mk_true();
            for( auto& wrpp : wrs ) {
              if( wrpp->tid != wrp->tid && !wrpp->is_update() ) {
                no_between = no_between &&
                  ( hb_encoding.mk_ghb_c11_mo(wrpp, wrp) ||
                    hb_encoding.mk_ghb_c11_mo( wr, wrpp) );
              }
            }
            z3::expr b = get_rel_seq_bvar( wrp, wr );
            rel_seq = rel_seq && implies(wr->guard && wrp->guard && no_between, b );
          }
        }
      }
    }

    for( const hb_enc::se_ptr& rd : rds ) {
      z3::expr some_rfs = z3.mk_false(); //c.bool_val(false);
      z3::expr rd_v = rd->get_rd_expr( v1 );
      for( const hb_enc::se_ptr& wr : wrs ) {
        if( wr->tid == rd->tid && ( wr==rd || !is_po( wr, rd ) )) continue;
        // enter here only if wr,rd from diff thread or wr --po-->rd
        z3::expr wr_v = wr->get_wr_expr( v1 );
        z3::expr b = get_rf_bvar( v1, wr, rd );
        some_rfs = some_rfs || b;
        z3::expr eq = ( rd_v == wr_v );
        // well formed
        wf = wf && implies( b, wr->guard && eq);

        z3::expr hb_w_rd(z3.c);
        if( is_po( wr, rd ) ) {
          hb_w_rd = z3.mk_true();
        }else{
          hb_w_rd = hb_encoding.mk_ghb_c11_hb( wr, rd );
        }

        // sw due to rf
        if( rd->is_at_least_acq() ) {
          if( wr->is_at_least_rls() ) {
            rf = rf && implies( b, hb_w_rd );
          }else{
            for( auto& it : p.rel_seq_map[wr] ) {
              std::string bname = std::get<0>(it);
              hb_enc::se_ptr wrp = std::get<1>(it);
              if( is_po( wrp, rd ) && rd != wrp ) continue;
              z3::expr b_wrp_wr = z3.c.bool_const(  bname.c_str() );
              z3::expr hb_wrp_rd = z3.mk_false();
              if( !is_po( rd, wrp ) && rd != wrp ) {
                hb_wrp_rd = hb_encoding.mk_ghb_c11_hb( wrp, rd );
              }
              rf = rf && implies( b && b_wrp_wr, hb_wrp_rd );
            }
          }
        }
        // if( rd->is_na() || wr->is_na() ) {
        //   if( !is_po( wr, rd ) ) {// if po ordered no need to do anything
        //     wmm_error( "c11 non atomics not supported!!" );
        //   }
        // }else
        if( rd->is_at_most_rlx() || wr->is_at_most_rlx() ) {
          if( !is_po( wr, rd ) ) {
            // if po does not order wr,rd, we need to say that the
            // rest does not order rd,wr
            z3::expr hb_rd_wr = hb_encoding.mk_hb_c11_hb( rd, wr );
            rf = rf && implies( b, ! hb_rd_wr );
          }
        }

        // coherence wr rw
        for( const hb_enc::se_ptr& wrp : wrs ) {
          if( wr == wrp ) continue;
          // todo: write false for obvious true true hb
          z3::expr hb_wp_rd(z3.c),hb_rd_wp(z3.c),sc_wp_rd(z3.c);
          z3::expr mo_wp_w(z3.c), mo_w_wp(z3.c), sc_hb_wp_w(z3.c);
          // if rd == wrp, it will work since is_po returns true
          if( is_po( rd, wrp ) ) {
            hb_wp_rd = z3.mk_false();
            hb_rd_wp = z3.mk_true();
            sc_wp_rd = z3.mk_false();
          }else if( is_po( wrp, rd ) ) {
            hb_wp_rd = z3.mk_true();
            hb_rd_wp = z3.mk_false();
            sc_wp_rd = z3.mk_true();
          }else{
            //todo : should guard be included
            hb_wp_rd = hb_encoding.mk_hb_c11_hb( wrp, rd );
            hb_rd_wp = hb_encoding.mk_hb_c11_hb( rd, wrp );
            if( rd->is_sc() && wrp->is_sc() )
              sc_wp_rd = hb_encoding.mk_hb_c11_sc( wrp, rd );
          }
          if( is_po( wrp, wr ) ) {
            mo_w_wp = z3.mk_false();
            mo_wp_w = z3.mk_true();
            sc_hb_wp_w = z3.mk_true();
          }else if( is_po( wr, wrp ) ) {
            mo_w_wp = z3.mk_true();
            mo_wp_w = z3.mk_false();
            sc_hb_wp_w = z3.mk_false();
          }else{
            mo_w_wp = hb_encoding.mk_hb_c11_mo( wr, wrp );
            mo_wp_w = hb_encoding.mk_hb_c11_mo( wrp, wr );
            if( wrp->is_sc() ) {
              if( wr->is_sc() )
                sc_hb_wp_w = hb_encoding.mk_hb_c11_sc( wrp, wr );
              else
                sc_hb_wp_w = !(z3::expr)hb_encoding.mk_hb_c11_hb( wr, wrp );
            }
          }
          fr = fr && implies( b && wrp->guard && mo_w_wp, !hb_wp_rd ) &&
                     implies( b && wrp->guard && mo_wp_w, !hb_rd_wp );
          // rr coherence
          for( const hb_enc::se_ptr& rdp : rds ) {
            if( rd == rdp || wrp == rdp ) continue;
            if( wrp->tid == rdp->tid && !is_po( wrp, rdp ) ) continue;
            z3::expr bp = get_rf_bvar( v1, wrp, rdp, false );
            z3::expr hb_rd_rdp(z3.c);
            if( is_po( rd, rdp ) ) {
              hb_rd_rdp = z3.mk_true();
            } else if( is_po( rdp, rd ) ) {
              hb_rd_rdp = z3.mk_false();
            }else{
              hb_rd_rdp = hb_encoding.mk_hb_c11_hb( rd, rdp );
            }
            fr = fr && implies( b && bp && mo_wp_w, !hb_rd_rdp );
          }

          // SCReads
          if( rd->is_sc() && wrp->is_sc() && wr != wrp ) {
            if( wr->is_sc() ) {
                fr = fr && implies( b && wrp->guard && sc_wp_rd, sc_hb_wp_w );
            }else{
              z3::expr no_else = z3.mk_true();
              for( auto& wrpp : wrs ) {
                if( !wrpp->is_sc() || wrpp == wrp ) continue;
                z3::expr sc_wpp_wp(z3.c);
                if( is_po( wrpp, wrp ) ) {
                  sc_wpp_wp = z3.mk_true();
                } else if( is_po( wrp, wrpp ) ) {
                  sc_wpp_wp = z3.mk_false();
                }else{
                  sc_wpp_wp = hb_encoding.mk_ghb_c11_sc( wrpp, wrp );
                }
                z3::expr sc_rd_wpp(z3.c);
                if( is_po( rd, wrpp ) ) {
                  sc_rd_wpp = z3.mk_true();
                } else if( is_po( wrpp, rd ) ) {
                  sc_rd_wpp = z3.mk_false();
                }else{
                  sc_rd_wpp = hb_encoding.mk_ghb_c11_sc( rd,  wrpp );
                }
                no_else = no_else &&
                  implies( wrpp->guard, ( sc_wpp_wp || sc_rd_wpp ) );
              }
              fr = fr &&
                implies( b && wrp->guard && sc_wp_rd && no_else, sc_hb_wp_w);
            }
          }
        }
      }
      wf = wf && implies( rd->guard, some_rfs );
    }

    // modification order
    auto it1 = wrs.begin();
    z3::expr_vector dists(z3.c);
    for( ; it1 != wrs.end() ; it1++ ) {
      const hb_enc::se_ptr& wr1 = *it1;
      auto it2 = it1;
      it2++;
      for( ; it2 != wrs.end() ; it2++ ) {
        const hb_enc::se_ptr& wr2 = *it2;
        // if( wr1->tid == wr2->tid && is_po( ) ) contiune;
        if( is_po( wr1, wr2 ) ) {
          fr = fr && hb_encoding.mk_ghb_c11_mo( wr1, wr2 );
        }else if( is_po( wr2, wr1 ) ) {
          fr = fr && hb_encoding.mk_ghb_c11_mo( wr2, wr1 );
        }else{
          //todo: should the following condition be gaurded?
          fr = fr
            && z3::implies( hb_encoding.mk_hb_c11_hb(wr1,wr2),
                            hb_encoding.mk_hb_c11_mo(wr1,wr2) )
            && z3::implies( hb_encoding.mk_hb_c11_hb(wr2,wr1),
                            hb_encoding.mk_hb_c11_mo(wr2,wr1) );
        }
      }
      dists.push_back( wr1->get_c11_mo_solver_symbol() );
    }
    if( dists.size() != 0)  ws = ws && distinct( dists );

    //non-atomic
    for(auto it1 = wrs.begin(); it1 != wrs.end() ; it1++ ) {
      const hb_enc::se_ptr& wr1 = *it1;
      auto it2 = it1; it2++;
      for( ; it2 != wrs.end() ; it2++ ) {
        const hb_enc::se_ptr& wr2 = *it2;
        if( wr1->tid == wr2->tid ) continue;
        if( is_po( wr1, wr2 ) || is_po( wr2, wr1 ) ) continue;

        if( wr1->is_na() || wr2->is_na() ) {
          z3::expr guard = wr1->guard && wr2->guard;
          z3::expr ordered = hb_encoding.mk_hb_c11_hb_ao( wr1, wr2 );
          // z3::expr ord_1_2 = hb_encoding.mk_hb_c11_hb( wr1, wr2 );
          // z3::expr ord_2_1 = hb_encoding.mk_hb_c11_hb( wr2, wr1 );
          // z3::expr ordered = ord_1_2 || ord_2_1;
          // p.append_property( z3::implies( guard, ordered ) );
          p.append_property( ordered );
        }
      }
      for( auto& rd : rds ) {
        if( wr1->tid == rd->tid ) continue;
        if( is_po( wr1, rd ) || is_po( rd, wr1 ) ) continue;
        if( rd->is_update() ) continue;
        if( wr1->is_na() || rd->is_na() ) {
          z3::expr guard = wr1->guard && rd->guard;
          z3::expr ordered = hb_encoding.mk_hb_c11_hb_ao( wr1, rd );
          // z3::expr ord_1_2 = hb_encoding.mk_hb_c11_hb( wr1, rd );
          // z3::expr ord_2_1 = hb_encoding.mk_hb_c11_hb( rd, wr1 );
          // z3::expr ordered = ord_1_2 || ord_2_1;
          p.append_property( z3::implies( guard, ordered ) );
        }
      }
    }
  }


  // sc events
  auto it1 = p.all_sc.begin();
  z3::expr_vector dists(z3.c);
  for( ; it1 != p.all_sc.end() ; it1++ ) {
    const hb_enc::se_ptr& e1 = *it1;
    auto it2 = it1; it2++;
    for( ; it2 != p.all_sc.end() ; it2++ ) {
      const hb_enc::se_ptr& e2 = *it2;
      if( is_po( e1, e2 ) ) {
        fr = fr && hb_encoding.mk_ghb_c11_sc( e1, e2 );
      }else if( is_po( e2, e1 ) ) {
        fr = fr && hb_encoding.mk_ghb_c11_sc( e2, e1 );
      }else if( e1->is_fence() && e2->is_fence() ) {
        z3::expr hb_12 = hb_encoding.mk_hb_c11_hb(e1,e2);
        z3::expr sc_12 = hb_encoding.mk_hb_c11_sc(e1,e2);
        z3::expr hb_21 = hb_encoding.mk_hb_c11_hb(e2,e1);
        z3::expr sc_21 = hb_encoding.mk_hb_c11_sc(e2,e1);
        fr = fr && hb_12 == sc_12  && hb_21 == sc_21;
      }else{
        fr = fr && z3::implies( hb_encoding.mk_hb_c11_hb( e1, e2 ),
                                hb_encoding.mk_hb_c11_sc( e1, e2 ) )
          && z3::implies( hb_encoding.mk_hb_c11_hb( e2, e1 ),
                          hb_encoding.mk_hb_c11_sc( e2, e1 ) );
      }
    }
    dists.push_back( e1->get_c11_sc_solver_symbol() );
  }
  if( dists.size() != 0) ws = ws && distinct( dists );
  fr = fr.simplify();
  wf = wf.simplify();
  rf = rf.simplify();
  grf = rf;
}
