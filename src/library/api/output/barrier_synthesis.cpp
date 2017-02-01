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
 *
 */

#include "program/program.h"
#include "output_exception.h"
#include "barrier_synthesis.h"
#include "cssa/wmm.h"
#include "hb_enc/cycles.h"
#include "api/output/output_base_utilities.h"
#include <chrono>
#include <algorithm>

using namespace tara;
using namespace tara::cssa;
using namespace tara::api::output;
using namespace tara::helpers;
using namespace std;


barrier_synthesis::barrier_synthesis(options& o_,
                                     helpers::z3interf& z3_)
  : output_base( o_, z3_)
  , normal_form( o_, z3_, true, false, false, false)
  , cycle_finder( z3_ )
{
  verbose = helpers::exists( o.mode_options, std::string("verbose") );
  // for(string p : o.mode_options) {
  //   if (p=="verbose") verbose = true;
  // }
}

void barrier_synthesis::init( const hb_enc::encoding& hb_encoding,
                              const z3::solver& sol_desired,
                              const z3::solver& sol_undesired,
                              std::shared_ptr< const tara::program > _program )
{
    output_base::init(hb_encoding, sol_desired, sol_undesired, _program);
    normal_form.init(hb_encoding, sol_desired, sol_undesired, _program);
    cycle_finder.program = _program.get();
    // program = _program;
}


void barrier_synthesis::find_cycles(nf::result_type& bad_dnf) {
  all_cycles.clear();
  all_cycles.resize( bad_dnf.size() );
  unsigned bad_dnf_num = 0;
  for( auto hbs : bad_dnf ) {
    vector<cycle>& cs = all_cycles[bad_dnf_num++];
    cycle_finder.find_cycles( hbs, cs );
    if( cs.size() == 0 ) {
      throw output_exception( "barrier synthesis: a conjunct without any cycles!!");
    }
  }

  if( verbose ) {
    std::cout << "Detected cycles:\n";
    print_all_cycles( std::cout );
  }

}

void barrier_synthesis::print_all_cycles( ostream& stream ) const {
  stream << "cycles found!\n";
  for( auto& cycles : all_cycles ) {
    stream << "[" << endl;
    for( auto& cycle : cycles ) {
      stream << cycle << endl;
    }
    stream << "]" << endl;
  }
}

void barrier_synthesis::report( ostream& stream,
                                const std::vector<hb_enc::se_ptr>& inserted,
                                std::string msg ) const {
  if( inserted.size() > 0 ) {
    stream << msg << ":-\n";
    for ( auto e : inserted ) {
      stream << "thread " << e->get_tid() <<  ":" << e->name() << endl;
    }
  }
}

void barrier_synthesis::print(ostream& stream, bool machine_readable) const {
  if( verbose && !machine_readable )
    normal_form.print(stream, false);

  stream << "The following synchromizations must be added:\n";

  report( stream, result_sc_fences , "sc fences after");
  report( stream, result_rlsacq_fences, "rlsacq fences after");
  report( stream, result_rls_fences, "rls fences after");
  report( stream, result_acq_fences, "acq fences after");
  report( stream, result_rls_upgrade    , "upgrade to rls");
  report( stream, result_acq_upgrade    , "upgrade to acq");
  report( stream, result_rlsacq_upgrade , "upgrade to rlsacq");

  report( stream, barrier_where , "full fences after");
  report( stream, soft_barrier_where , "soft fences after");

  // stream <<"Barriers must be inserted after the following instructions:- \n";
  // for ( unsigned i = 0; i < barrier_where.size(); i++ ) {
  //   hb_enc::se_ptr e = barrier_where[i];
  //   stream << "thread " << e->get_tid() <<  ":" << e->name() << endl;
  // }
  // stream <<"Soft Barriers must be inserted after the following instructions:- \n";
  // for ( unsigned i = 0; i < soft_barrier_where.size(); i++ ) {
  //   hb_enc::se_ptr e = soft_barrier_where[i];
  //   stream << "thread " << e->get_tid() << e->name() << endl;
  // }

  stream << endl;
}

void barrier_synthesis::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  metric.additional_info = info;
}


//----------------------------------------------------------------------------



z3::expr barrier_synthesis::get_h_var_bit( const hb_enc::se_ptr& b,
                                           const hb_enc::se_ptr& e_i,
                                           const hb_enc::se_ptr& e_j ) {
  auto pr = std::make_tuple( b, e_i, e_j );
  auto it = hist_map.find( pr );
  if( it != hist_map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool( b->name() + "_"+ e_i->name() + "_"+ e_j->name() );
  hist_map.insert( std::make_pair( pr, bit) );
  return bit;
}

z3::expr barrier_synthesis::get_write_order_bit( const hb_enc::se_ptr& b,
                                                 const hb_enc::se_ptr& e ) {
  auto pr = std::make_pair( b, e);
  auto it = wr_ord_map.find( pr );
  if( it != wr_ord_map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool();
  wr_ord_map.insert( std::make_pair( pr, bit) );
  return bit;
}

z3::expr barrier_synthesis::get_bit_from_map(std::map< std::pair<hb_enc::se_ptr,hb_enc::se_ptr>, z3::expr >& map, const hb_enc::se_ptr& b, const hb_enc::se_ptr& e ) {
  auto pr = std::make_pair( b, e);
  auto it = map.find( pr );
  if( it != map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool();
  map.insert( std::make_pair( pr, bit) );
  return bit;
}

z3::expr barrier_synthesis::get_bit_from_map(std::map< std::tuple<hb_enc::se_ptr,hb_enc::se_ptr, cssa::variable>, z3::expr >& map, const hb_enc::se_ptr& b, const hb_enc::se_ptr& e, const cssa::variable& v ) {
  auto pr = std::make_tuple( b, e, v);
  auto it = map.find( pr );
  if( it != map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool();
  map.insert( std::make_pair( pr, bit) );
  return bit;
}


z3::expr barrier_synthesis::get_sc_var_bit( const hb_enc::se_ptr& b,
                                            const hb_enc::se_ptr& e ) {
  return get_bit_from_map( sc_var_map, b, e );
}

z3::expr barrier_synthesis::get_rls_var_bit( const hb_enc::se_ptr& b,
                                            const hb_enc::se_ptr& e ) {
  return get_bit_from_map( rls_var_map, b, e );
}

z3::expr barrier_synthesis::get_acq_var_bit( const hb_enc::se_ptr& b,
                                             const hb_enc::se_ptr& e ) {
  return get_bit_from_map( acq_var_map, b, e );
}

z3::expr barrier_synthesis::get_rlsacq_var_bit( const hb_enc::se_ptr& b,
                                                const hb_enc::se_ptr& e ) {
  return get_bit_from_map( rlsacq_var_map, b, e );
}

z3::expr barrier_synthesis::get_rls_var_bit( const hb_enc::se_ptr& b,
                                             const hb_enc::se_ptr& e,
                                             const cssa::variable& v) {
  return get_bit_from_map( rls_v_var_map, b, e, v);
}

z3::expr barrier_synthesis::get_acq_var_bit( const hb_enc::se_ptr& b,
                                             const hb_enc::se_ptr& e,
                                             const cssa::variable& v) {
  return get_bit_from_map( acq_v_var_map, b, e, v);
}

z3::expr barrier_synthesis::get_rlsacq_var_bit( const hb_enc::se_ptr& b,
                                                const hb_enc::se_ptr& e,
                                                const cssa::variable& v) {
  return get_bit_from_map( rlsacq_v_var_map, b, e, v);
}

//=====

z3::expr barrier_synthesis::get_rls_bit( const hb_enc::se_ptr& e,
                                         const cssa::variable& v ) {
  if( e->prog_v != v || !e->is_wr() ) return z3.mk_false();
  if( e->is_at_least_rls() ) return z3.mk_true();
  return rls_v_map.at( e );
}

z3::expr barrier_synthesis::get_acq_bit( const hb_enc::se_ptr& e,
                                         const cssa::variable& v ) {
  if( e->prog_v != v  || !e->is_rd() ) return z3.mk_false();
  if( e->is_at_least_acq() ) return z3.mk_true();
  return acq_v_map.at( e );
}

z3::expr barrier_synthesis::get_rlsacq_bit( const hb_enc::se_ptr& e,
                                            const cssa::variable& v ) {
  if( e->prog_v != v || !e->is_update() ) return z3.mk_false();
  if( e->is_at_least_rlsacq() ) return z3.mk_true();
  return rlsacq_v_map.at( e );
}


// we need to choose
// constraints for mk_edge( b, a)
// h_bij means that b~>i~>j and b,j is ordered at i
// h_bbb => true
// h_bbj => false   j != b ( TODO <<------------- check )
// h_bij => /\_{k \in prev(i)} ( h_bkj  \/
//                               ( h_bkk /\ check_ppo(k,j) ) \/
//                               ( l_k /\ w_bk /\ is_write(j) ) \/
//                               b_k )
// w_bb => is_write(b)
// w_bi => (/\_{k \in prev(i)} (w_bk)) \/ (h_bii /\ is_write(i) )
// b_k => l_k
// return h_baa

z3::expr barrier_synthesis::mk_edge_constraint( hb_enc::se_ptr& b,
                                                hb_enc::se_ptr& a,
                                                z3::expr& hard ) {
  assert( a->tid == b->tid );
  mm_t mm = program->get_mm();
  hb_enc::se_tord_set pending;
  hb_enc::se_vec found;
  // std::map< std::tuple<hb_enc::se_ptr,hb_enc::se_ptr,hb_enc::se_ptr>,z3::expr >
  //   history_map;
  std::map< hb_enc::se_ptr, z3::expr > forced_map;

  pending.insert( b );
  while( !pending.empty() ) {
    hb_enc::se_ptr e = *pending.begin();
    pending.erase( e );
    // hb_enc::se_ptr e = helpers::pick( pending );
    found.push_back( e );
    if( e->get_topological_order() > a->get_topological_order() )
      continue;
    if( a == e ) break;
    for( auto& xpp : e->post_events ) {
      if( !helpers::exists( found, xpp.e ) ) pending.insert( xpp.e );
    }
  }

  for( auto it = found.begin(); it != found.end();it++ ) {
    hb_enc::se_ptr& i = *it;
    auto it2 = it;
    for(; it2 != found.end() ; it2++ ) {
      hb_enc::se_ptr& j = *it2;
      z3::expr conj = z3.mk_true();
      for( const hb_enc::se_ptr& k : i->prev_events ) {
        if( helpers::exists( found, k )  ) {
          z3::expr h_bkj = get_h_var_bit( b, k, j );
          z3::expr h_bkk = z3.mk_false();
          h_bkk = cssa::wmm_event_cons::check_ppo(mm,k,j) ?
            get_h_var_bit(b,k,k) : z3.mk_false();
          // todo: check for the lwsync requirement
          z3::expr lw_k = z3.mk_false();
          if( enable_lw_fences ) { // disable 
            z3::expr lw_k = j->is_wr()? get_lw_fence_bit( k ) : z3.mk_false();
            lw_k = lw_k && get_write_order_bit( b, k );
          }
          z3::expr b_k = get_fence_bit( k );
          conj = conj && (h_bkj || h_bkk || lw_k || b_k);
        }
      }
      if( i == b && j != b ) {
        conj = z3.mk_false();
      }
      z3::expr h_bij = get_h_var_bit( b, i, j );;
      hard = hard && implies( h_bij, conj );
    }
    if( enable_lw_fences ) {
      z3::expr conj = z3.mk_true();
      for( const hb_enc::se_ptr& k : i->prev_events ) {
        if( helpers::exists( found, k ) ) {
          conj = conj && get_write_order_bit( b, k );
        }
      }
      // todo: check for the lwsync requirement
      z3::expr h_bii = i->is_wr() ? get_h_var_bit( b, i, i ) : z3.mk_false();
      z3::expr w_bi = get_write_order_bit( b, i );
      hard = hard && implies( w_bi, conj ||  h_bii );
      z3::expr s_i = get_fence_bit( i );
      z3::expr lw_i = get_lw_fence_bit( i );
      hard = hard && implies( s_i, lw_i );
    }
  }
  return get_h_var_bit( b, a, a );

  assert( false );
  return z3.mk_false(); //todo : may be reachable for unreachable pairs
}

void barrier_synthesis::mk_c11_edge_constraint( hb_enc::se_ptr& b,
                                                hb_enc::se_ptr& a,
                                                z3::expr& hard ) {
  assert( b->tid == a->tid );
  if( helpers::exists( const_already_made, {b,a}) ) return;

  hb_enc::se_tord_set pending;
  hb_enc::se_vec found;

  pending.insert( b );
  while( !pending.empty() ) {
    hb_enc::se_ptr e = *pending.begin();
    pending.erase( e );
    // hb_enc::se_ptr e = helpers::pick( pending );
    found.push_back( e );
    if( e->get_topological_order() > a->get_topological_order() )
      continue;
    if( a == e ) break;
    for( auto& xpp : e->post_events ) {
      if( !helpers::exists( found, xpp.e ) ) pending.insert( xpp.e );
    }
  }

  for( auto it = found.begin(); it != found.end();it++ ) {
    hb_enc::se_ptr& i = *it;
    if( helpers::exists( const_already_made, {b,i}) ) continue;
    const_already_made.insert( {b,i} );
    z3::expr conj_sc  = z3.mk_true();
    z3::expr conj_rls = z3.mk_true();
    z3::expr conj_acq = z3.mk_true();
    std::map<cssa::variable, z3::expr> conj_rls_map;
    std::map<cssa::variable, z3::expr> conj_acq_map;
    for( auto v : program->globals ) {
      auto pr = std::make_pair(v, z3.mk_true() );
      conj_rls_map.insert( pr  );
      conj_acq_map.insert( pr );
    }
    for( const hb_enc::se_ptr& k : i->prev_events ) {
      if( helpers::exists( found, k ) ) {
        if( !k->is_sc() || !k->is_fence() ) {
          conj_sc = conj_sc && (get_sc_var_bit(b, k) || get_sc_fence_bit(k));
        }
        if( !k->is_at_least_rls() || !k->is_fence() ) {
          conj_rls = conj_rls
            && (get_rls_var_bit(b, k) || get_rls_fence_bit(k)
                || get_rlsacq_fence_bit( k ));
        }
        if( !k->is_at_least_acq() || !k->is_fence() ) {
          conj_acq = conj_acq
            && ( get_acq_var_bit(b, k) || get_acq_fence_bit(k)
                 || get_rlsacq_fence_bit(k) );
        }
        std::map<cssa::variable, z3::expr> conj_rls_map_copy;
        std::map<cssa::variable, z3::expr> conj_acq_map_copy;
        for( auto& v : program->globals ) {
          z3::expr d1 = conj_rls_map.at(v);
          if( !k->is_at_least_rls() || !k->is_wr() || k->prog_v != v ) {
            d1 = d1 && ( get_rls_var_bit(b, k, v) || get_rls_bit(k, v) ||
                       get_rlsacq_bit(k, v) || get_rls_fence_bit(k) ||
                       get_rlsacq_fence_bit(k) );
          }
          conj_rls_map_copy.insert( {v, d1} );
          z3::expr d2 = conj_acq_map.at(v);
          if( !k->is_at_least_acq() || !k->is_rd() || k->prog_v != v ) {
            d2 = d2 && (get_acq_var_bit( b, k, v ) || get_acq_bit( k, v )
                        || get_rlsacq_bit( k, v )
                        || get_acq_fence_bit(k) || get_rlsacq_fence_bit(k));
          }
          conj_acq_map_copy.insert( {v, d2} );
        }
        conj_rls_map = conj_rls_map_copy;
        conj_acq_map = conj_acq_map_copy;
      }
    }
    if( i == b ) {
      conj_sc  = z3.mk_false();
      conj_rls = z3.mk_false();
      conj_acq = z3.mk_false();
      std::map<cssa::variable, z3::expr> conj_rls_map_copy;
      std::map<cssa::variable, z3::expr> conj_acq_map_copy;
      for( auto& v : program->globals ) {
        conj_rls_map_copy.insert( { v, get_rls_bit( b, v ) } );
        conj_acq_map_copy.insert( { v, get_acq_bit( b, v ) } );

        // z3::expr bit = get_rls_bit( b, v );
        // auto pr = std::make_pair(v, bit );
        // conj_rls_map.insert( pr );
        // std::cout << conj_rls_map.at(v) << bit << "\n";
        // conj_rls_map.insert( { v, get_rls_bit( b, v ) } );
        // conj_acq_map.insert( { v, get_acq_bit( b, v ) } );
        //<< get_acq_bit( b, v ) << "\n";
      }
    // for( auto& it : conj_rls_map_copy ){std::cout << it.first << it.second << "\n";}
    // for( auto& it : conj_acq_map_copy ){std::cout << it.first << it.second << "\n";}
      conj_rls_map = conj_rls_map_copy;
      conj_acq_map = conj_acq_map_copy;
    // for( auto& it : conj_rls_map ){std::cout << it.first << it.second << "\n";}
    // for( auto& it : conj_acq_map ){std::cout << it.first << it.second << "\n";}
    }
    z3::expr cs = implies( get_sc_var_bit ( b, i ), conj_sc );
    cs = cs && implies( get_rls_var_bit( b, i ), conj_rls );
    cs = cs && implies( get_acq_var_bit( b, i ), conj_acq );
    for( auto& v : program->globals ) {
      cs = cs && implies( get_rls_var_bit( b, i, v ), conj_rls_map.at(v) );
      cs = cs && implies( get_acq_var_bit( b, i, v ), conj_acq_map.at(v) );
    }
    hard = hard && cs;
  }
}
// three possibilites rf,hb,ppo
z3::expr barrier_synthesis::
mk_c11_segment_constraint( std::vector<cycle_edge>& segment,
                           z3::expr& hard, bool sc_fence_needed ) {
  assert( segment.size() > 0 );
  z3::expr r_conj = z3.mk_true();
  std::vector< std::pair<cycle_edge,cycle_edge> > rf_segs;
  std::vector< cycle_edge > pos;
  bool rf_open = false;
  cycle_edge last_po(segment[0].before,segment[0].before,edge_type::ppo);
  if( segment[0].before == segment.back().after ) {
    //in case segment is a cycle
    if( segment.back().type  == edge_type::ppo ) {
      last_po = segment.back();
    }
  }
  for( auto& ed_s : segment ) {
    if( ed_s.type == edge_type::rf ) {
      rf_open = true;
    }else {
      if( rf_open == true ) {
        rf_open = false;
        if( ed_s.type == edge_type::hb ) {
          cycle_edge ed_end(ed_s.before,ed_s.before,edge_type::ppo);
          rf_segs.push_back( {last_po, ed_end} );
        }else{
          rf_segs.push_back( {last_po, ed_s} );
        }
      }
      if( ed_s.type == edge_type::ppo ) {
        pos.push_back( ed_s );
        last_po = ed_s;
      }else{
        last_po.before = last_po.after = ed_s.after ;
      }
    }
  }

  if( rf_open == true ) {
    cycle_edge ed_end(segment.back().after,segment.back().after,edge_type::ppo);
    if( segment[0].before == segment.back().after ) {
      //in case segment is a cycle
      if( segment[0].type  == edge_type::ppo ) {
        last_po = segment[0];
      }
    }
    rf_segs.push_back( {last_po, ed_end} );
  }

  if(verbose) {
    tara::hb_enc::debug_print( std::cerr, segment );  std::cerr << "\n";
    std::cerr << "rf pre posts:\n";
    for( auto& it : rf_segs  ) {
      tara::hb_enc::debug_print( std::cerr, it.first  ); std::cerr << ",";
      tara::hb_enc::debug_print( std::cerr, it.second ); std::cerr << "\n";
    }
    std::cerr << "po segments:\n";
    for( auto& po : pos  ) {
      tara::hb_enc::debug_print( std::cerr, po ); std::cerr << "\n";
    }
  }

  for( auto& it : rf_segs  ) {
    auto& ed1 = it.first; auto& ed2 = it.second;
    mk_c11_edge_constraint( ed1.before, ed1.after, hard );
    mk_c11_edge_constraint( ed2.before, ed2.after, hard );
    auto& v = ed2.before->prog_v;
    r_conj = r_conj && get_rls_var_bit(ed1.before,ed1.after,v) &&
      get_acq_var_bit(ed2.before,ed2.after,v);
  }

  if( sc_fence_needed ) {
    z3::expr sc_seg_disj = z3.mk_false();
    for( auto& ed : pos ) {
      mk_c11_edge_constraint( ed.before, ed.after, hard );
      sc_seg_disj = sc_seg_disj || get_sc_var_bit( ed.before, ed.after );
    }
    r_conj = r_conj && sc_seg_disj;
  }

  return r_conj;

  //old code
  // assert( segment.size() > 0 );
  // z3::expr r_conj = z3.mk_true();
  // auto& s = segment[0].before;
  // cycle_edge last_po(s,s,edge_type::ppo);
  // mk_c11_edge_constraint( last_po.before, last_po.after, hard );
  // z3::expr sc_seg_disj = z3.mk_false();
  // bool rf_open = false;
  // for( auto& ed_s : segment ) {
  //   if( ed_s.type == edge_type::rf ) {
  //     rf_open = true;
  //   } else {
  //     if( rf_open == true ) {
  //       rf_open = false;
  //       auto& v = ed_s.before->prog_v;
  //       r_conj = r_conj &&
  //         get_rls_var_bit( last_po.before, last_po.after, v ) &&
  //         get_acq_var_bit( ed_s.before   , ed_s.after, v );
  //     }
  //     sc_seg_disj = sc_seg_disj || get_sc_var_bit( ed_s.before, ed_s.after );
  //     last_po = ed_s;
  //     mk_c11_edge_constraint( last_po.before, last_po.after, hard );
  //   }
  // }
  // if( rf_open == true ) {
  //   auto& l = segment.back().after;
  //   cycle_edge ed_s(l,l,edge_type::ppo);
  //   auto& v = ed_s.before->prog_v;
  //   mk_c11_edge_constraint( ed_s.before, ed_s.after, hard );
  //   r_conj = r_conj &&
  //     get_rls_var_bit( last_po.before, last_po.after, v ) &&
  //     get_acq_var_bit( ed_s.before   , ed_s.after, v );
  // }
  // if( sc_fence_needed ) r_conj = r_conj && sc_seg_disj;
  // return r_conj;
}

z3::expr barrier_synthesis::mk_cycle_constraint(cycle& cycle, z3::expr& hard) {
  z3::expr r_conj = z3.mk_true();
  if( program->is_mm_c11() ) {
    assert( cycle.size() > 1 );
    bool prefix_mode = true;
    bool sc_fence_needed = false; // sc fences needed only if
                                   // there are more than two fr in the cycle.
    std::vector<cycle_edge> prefix;
    std::vector<cycle_edge> segment;
    for( auto& ed : cycle.edges ) {
      if( ed.type == edge_type::fr ) {
        if( prefix_mode == true ) {
          prefix_mode = false;
        }else{
          r_conj = r_conj && mk_c11_segment_constraint( segment, hard);
          sc_fence_needed = true;
          segment.clear();
        }
      }else{
        if( prefix_mode == true ) {
          prefix.push_back( ed );
        }else{
          segment.push_back( ed );
        }
      }
    }
    segment.insert(segment.end(), prefix.begin(), prefix.end());
    r_conj = r_conj && mk_c11_segment_constraint(segment, hard,sc_fence_needed);
  }else{
    for( auto edge : cycle.edges ) {
      if( edge.type == edge_type::rpo ) {
        r_conj = r_conj && mk_edge_constraint( edge.before, edge.after, hard );
      }
    }
  }
  return r_conj;
}

z3::expr barrier_synthesis::
create_sync_bit( std::map< hb_enc::se_ptr, z3::expr >& s_map,
                 const std::string prefix,
                 const hb_enc::se_ptr& e,
                 std::vector<z3::expr>& soft ) {
  z3::expr bit = z3.get_fresh_bool(prefix+e->name());
  s_map.insert({ e, bit });
  soft.push_back( !bit );
  return bit;
}
void barrier_synthesis::gen_max_sat_problem_new() {
  // z3::context& z3_ctx = sol_bad->ctx();
  const_already_made.clear();
  z3::expr hard = z3.mk_true();

  for( unsigned t = 0; t < program->size(); t++ ) {
     const auto& thread = program->get_thread( t );
     for( const auto& e : thread.events ) {
       // todo : optimize the number of introduced variables
       if( !program->is_mm_c11() ) {
         z3::expr m_bit = z3.get_fresh_bool(e->name());
         fence_map.insert({ e, m_bit });
         soft.push_back( !m_bit );
         z3::expr s_bit = z3.get_fresh_bool("lw_"+e->name());
         light_fence_map.insert({ e, s_bit });
         soft.push_back( !s_bit );
       }else{
         //               /--> rls ----------> rls v
         // sc -> rlsacq -|                    ^
         //               |--> acq -> acq v    |
         //               |               ^    |
         //               |               |    |
         //               \-------------> rlsacq v
         auto sc_b = create_sync_bit(sc_fence_map,"sc_",e, soft );
         auto rlsacq_b=create_sync_bit(rlsacq_fence_map,"rlsacq_",e,soft);
         auto rls_b = create_sync_bit(rls_fence_map,"rls_",e, soft );
         auto acq_b = create_sync_bit(acq_fence_map,"acq_",e, soft );
         hard = hard && implies(sc_b, rlsacq_b) && implies(rlsacq_b, rls_b)
           && implies(rlsacq_b, acq_b);
         if( e->is_wr() && !e->is_at_least_rls() ) {
           auto rls_v_b=create_sync_bit(rls_v_map,"rls_v_",e, soft);
           hard = hard && implies(rls_b, rls_v_b);
         }
         if( e->is_rd() && !e->is_at_least_acq() ) {
           auto acq_v_b=create_sync_bit(acq_v_map,"acq_v_",e, soft);
           hard = hard && implies(acq_b, acq_v_b);
         }
         if( e->is_update() && !e->is_at_least_rlsacq() ) {
           auto rlsacq_v_b=create_sync_bit(rlsacq_v_map,"rlsacq_v_",e,soft);
           hard = hard && implies( rlsacq_b,   rlsacq_v_b );
           hard = hard && implies( rlsacq_v_b, get_rls_bit(e,e->prog_v) );
           hard = hard && implies( rlsacq_v_b, get_acq_bit(e,e->prog_v) );
         }

         // z3::expr sc_bit = z3.get_fresh_bool("sc_"+e->name());
         // sc_fence_map.insert({ e, sc_bit });
         // soft.push_back( !sc_bit );
         // z3::expr rls_bit = z3.get_fresh_bool("rls_"+e->name());
         // rls_fence_map.insert({ e, rls_bit });
         // soft.push_back( !rls_bit );
         // z3::expr acq_bit = z3.get_fresh_bool("acq_"+e->name());
         // acq_fence_map.insert({ e, acq_bit });
         // soft.push_back( !acq_bit );
         // z3::expr rlsacq_bit = z3.get_fresh_bool("rlsacq_"+e->name());
         // rlsacq_fence_map.insert({ e, rlsacq_bit });
         // soft.push_back( !rlsacq_bit );
         // if( e->is_wr() && !e->is_rls() ) {
         //   z3::expr rls_bit = z3.get_fresh_bool("rls_v_"+e->name());
         //   rls_v_map.insert({ e, rls_bit });
         //   soft.push_back( !rls_bit );
         // }
         // if( e->is_rd() && !e->is_acq() ) {
         //   z3::expr acq_bit = z3.get_fresh_bool("acq_v_"+e->name());
         //   acq_v_map.insert({ e, acq_bit });
         //   soft.push_back( !acq_bit );
         // }
         // if( e->is_update() && !e->is_rlsacq() ) {
         //   z3::expr rlsacq_bit = z3.get_fresh_bool("rlsacq_v_"+e->name());
         //   rlsacq_v_map.insert({ e, rlsacq_bit });
         //   soft.push_back( !rlsacq_bit );
         // }
         // hard = hard && implies(sc_bit, rls_bit)
         //   && implies(sc_bit, acq_bit) && implies(sc_bit, rlsacq_bit);
       }
     }
  }

  for( auto& cycles : all_cycles ) {
    if( cycles.size() == 0 ) continue; // throw error unfixable situation
    z3::expr c_disjunct = z3.mk_false();//z3_ctx.bool_val(false);
    for( auto& cycle : cycles ) {
      z3::expr r_conj = mk_cycle_constraint( cycle, hard );
      z3::expr c = z3.get_fresh_bool();
      c_disjunct = c_disjunct || c;
      hard = hard && implies( c, r_conj );
    }
    hard = hard && c_disjunct;
  }
  cut.push_back( hard );

}

// ===========================================================================
// max sat code

void barrier_synthesis::assert_soft_constraints( z3::solver&s ,
                                                 std::vector<z3::expr>& cnstrs,
                                                 std::vector<z3::expr>& aux_vars
                                                 ) {
  for( auto f : cnstrs ) {
    auto n = z3.get_fresh_bool();
    aux_vars.push_back(n);
    s.add( f || n ) ;
  }
}

z3::expr barrier_synthesis:: at_most_one( z3::expr_vector& vars ) {
  z3::expr result = vars.ctx().bool_val(true);
  if( vars.size() <= 1 ) return result;
  // todo check for size 0
  z3::expr last_xor = vars[0];
  
  for( unsigned i = 1; i < vars.size(); i++ ) {
    z3::expr curr_xor = (vars[i] != last_xor );
    result = result && (!last_xor || curr_xor);
    last_xor = curr_xor;
  }

  return result;
}

int barrier_synthesis:: fu_malik_maxsat_step( z3::solver &s,
                                              std::vector<z3::expr>& soft_cnstrs,
                                              std::vector<z3::expr>& aux_vars ) {
    z3::expr_vector assumptions(z3.c);
    z3::expr_vector core(z3.c);
    for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
      assumptions.push_back(!aux_vars[i]);
    }
    if (s.check(assumptions) != z3::check_result::unsat) {
      return 1; // done
    }else {
      core=s.unsat_core();
      // std::cout << core.size() << "\n";
      z3::expr_vector block_vars(z3.c);
      // update soft-constraints and aux_vars
      for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
        unsigned j;
        // check whether assumption[i] is in the core or not
        for( j = 0; j < core.size(); j++ ) {
          if( assumptions[i] == core[j] )
            break;
        }
        if (j < core.size()) {
          z3::expr block_var = z3.get_fresh_bool();
          z3::expr new_aux_var = z3.get_fresh_bool();
          soft_cnstrs[i] = ( soft_cnstrs[i] || block_var );
          aux_vars[i]    = new_aux_var;
          block_vars.push_back( block_var );
          s.add( soft_cnstrs[i] || new_aux_var );
        }
      }
      z3::expr at_most_1 = at_most_one( block_vars );
      s.add( at_most_1 );
      return 0; // not done.
    }
}

z3::model
barrier_synthesis::fu_malik_maxsat( z3::expr hard,
                                    std::vector<z3::expr>& soft_cnstrs ) {
    unsigned k;
    z3::solver s(z3.c);
    s.add( hard );
    assert( s.check() != z3::unsat );
    assert( soft_cnstrs.size() > 0 );

    std::vector<z3::expr> aux_vars;
    assert_soft_constraints( s, soft_cnstrs, aux_vars );
    k = 0;
    for (;;) {
      if( fu_malik_maxsat_step(s, soft_cnstrs, aux_vars) ) {
    	  z3::model m=s.get_model();
    	  return m;
      }
      k++;
    }
}

// ===========================================================================
// main function for synthesis


void barrier_synthesis::output(const z3::expr& output) {
    output_base::output(output);
    normal_form.output(output);
    
    nf::result_type bad_dnf = normal_form.get_result(true, true);

    // measure time
    auto start_time = chrono::steady_clock::now();

    if( verbose )
      normal_form.print( std::cout, false);

    find_cycles( bad_dnf );

    // gen_max_sat_problem();
    gen_max_sat_problem_new();

    if( verbose ) {
      std::cout << cut[0] << endl;
      std::cout << "soft:" << endl;
      for( auto s : soft ) {
        std::cout << s << endl;
      }
    }

    if( soft.size() ==  0 ) {
      throw output_exception( "No relaxed edge found in any cycle!!");
      return;
    }
    z3::model m =fu_malik_maxsat( cut[0], soft );
    // std::cout << m ;

    for( auto it=fence_map.begin(); it != fence_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      z3::expr b = it->second;
      if( m.eval(b).get_bool() ) {
        barrier_where.push_back( e );
      }else{
        z3::expr b_s = light_fence_map.at(e);
        if( m.eval(b_s).get_bool() ) {
          soft_barrier_where.push_back( e );
        }
      }
    }

    for( auto it=sc_fence_map.begin(); it != sc_fence_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      if( m.eval(it->second).get_bool() ) {
        result_sc_fences.push_back( e );
      }else if( m.eval( rlsacq_fence_map.at(e) ).get_bool() ) {
        result_rlsacq_fences.push_back( e );
      }else if( m.eval( rls_fence_map.at(e) ).get_bool() ) {
        result_rls_fences.push_back( e );
      }else if( m.eval( acq_fence_map.at(e) ).get_bool() ) {
        result_acq_fences.push_back( e );
      }
    }

    for( auto it= rlsacq_v_map.begin(); it != rlsacq_v_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      if( m.eval(it->second).get_bool() ) {
        result_rlsacq_upgrade.push_back(e);
      }
    }

    for( auto it= rls_v_map.begin(); it != rls_v_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      if( m.eval(it->second).get_bool() ) {
        result_rls_upgrade.push_back(e);
      }
    }

    for( auto it= acq_v_map.begin(); it != acq_v_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      if( m.eval(it->second).get_bool() ) {
        result_acq_upgrade.push_back(e);
      }
    }


    // for( auto it=segment_map.begin(); it != segment_map.end(); it++ ) {
    //   hb_enc::se_ptr e = it->first;
    //   z3::expr b = it->second;
    //   if( m.eval(b).get_bool() )
    //     barrier_where.push_back( e );
    // }

    info = to_string(all_cycles.size()) + " " +
      to_string(barrier_where.size()) + " " + to_string(soft_barrier_where.size());

    auto delay = chrono::steady_clock::now() - start_time;
    time = chrono::duration_cast<chrono::microseconds>(delay).count();
}


// ===========================================================================
// old code ready for deletion

bool cmp( hb_enc::se_ptr a, hb_enc::se_ptr b ) {
  return ( a->get_instr_no() < b->get_instr_no() );
}


void barrier_synthesis::gen_max_sat_problem() {
  z3::context& z3_ctx = sol_bad->ctx();

  for( auto& cycles : all_cycles ) {
      for( auto& cycle : cycles ) {
        bool found = false;
        for( auto edge : cycle.edges ) {
          if( edge.type==edge_type::rpo ) {
            //check each edge for rpo: push them in another vector
            //compare the edges in same thread
            tid_to_se_ptr[ edge.before->tid ].push_back( edge.before );
            tid_to_se_ptr[ edge.before->tid ].push_back( edge.after );
            found = true;
          }
        }
        if( !found )
          throw output_exception( "cycles found without any relaxed program oders!!");
      }
  }

  for( auto it = tid_to_se_ptr.begin();  it != tid_to_se_ptr.end(); it++ ) {
    hb_enc::se_vec& vec = it->second;
    std::sort( vec.begin(), vec.end(), cmp );
    vec.erase( std::unique( vec.begin(), vec.end() ), vec.end() );
    for(auto e : vec ) {
      z3::expr s = z3.get_fresh_bool();
      // std::cout << s << e->name() << endl;
      segment_map.insert( {e,s} );
      soft.push_back( !s);
    }
  }

  z3::expr hard = z3_ctx.bool_val(true);
  for( auto& cycles : all_cycles ) {
    if( cycles.size() == 0 ) continue; // throw error unfixable situation
    z3::expr c_disjunct = z3.mk_false();
    for( auto& cycle : cycles ) {
      z3::expr r_conjunct = z3_ctx.bool_val(true);
      for( auto edge : cycle.edges ) {
        if( edge.type==edge_type::rpo ) {
          unsigned tid = edge.before->tid;
          auto& events = tid_to_se_ptr.at(tid);
          bool in_range = false;
          z3::expr s_disjunct = z3.mk_false();
          for( auto e : events ) {
            if( e == edge.before ) in_range = true;
            if( e == edge.after ) break;
            if( in_range ) {
              auto find_z3 = segment_map.find( e );
              s_disjunct = s_disjunct || find_z3->second;
            }
          }
          r_conjunct = r_conjunct && s_disjunct;
        }
      }
      z3::expr c = z3.get_fresh_bool();
      // z3_to_cycle.insert({c, &cycle});
      c_disjunct=c_disjunct || c;
      hard = hard && implies( c, r_conjunct );
    }
    hard = hard && c_disjunct;
  }
  cut.push_back( hard );

}
