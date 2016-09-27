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


barrier_synthesis::barrier_synthesis(helpers::z3interf& z3_,
                                     bool verify, bool _verbose)
  : output_base( z3_), verbose(_verbose)
  , normal_form(z3_, true, false, false, false, verify)
  , cycle_finder( z3_ )
{}

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

void barrier_synthesis::print(ostream& stream, bool machine_readable) const {
  if( verbose && !machine_readable )
    normal_form.print(stream, false);


  stream <<"Barriers must be inserted after the following instructions:- \n";
  for ( unsigned i = 0; i < barrier_where.size(); i++ ) {
    hb_enc::se_ptr e = barrier_where[i];
    stream << "thread " << e->get_tid() << e->name() << endl;
  }
  stream <<"Soft Barriers must be inserted after the following instructions:- \n";
  for ( unsigned i = 0; i < soft_barrier_where.size(); i++ ) {
    hb_enc::se_ptr e = soft_barrier_where[i];
    stream << "thread " << e->get_tid() << e->name() << endl;
  }

  stream << endl;
}

void barrier_synthesis::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  metric.additional_info = info;
}


//----------------------------------------------------------------------------

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
    z3::expr c_disjunct = z3_ctx.bool_val(false);
    for( auto& cycle : cycles ) {
      z3::expr r_conjunct = z3_ctx.bool_val(true);
      for( auto edge : cycle.edges ) {
        if( edge.type==edge_type::rpo ) {
          unsigned tid = edge.before->tid;
          auto& events = tid_to_se_ptr.at(tid);
          bool in_range = false;
          z3::expr s_disjunct = z3_ctx.bool_val(false);
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


z3::expr barrier_synthesis::get_h_var_bit( hb_enc::se_ptr b,
                                           hb_enc::se_ptr e_i,
                                           hb_enc::se_ptr e_j ) {
  auto pr = std::make_tuple( b, e_i, e_j );
  auto it = hist_map.find( pr );
  if( it != hist_map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool( b->name() + "_"+ e_i->name() + "_"+ e_j->name() );
  hist_map.insert( std::make_pair( pr, bit) );
  return bit;
}

z3::expr barrier_synthesis::get_write_order_bit( hb_enc::se_ptr b,
                                                 hb_enc::se_ptr e ) {
  auto pr = std::make_pair( b, e);
  auto it = wr_ord_map.find( pr );
  if( it != wr_ord_map.end() )
    return it->second;
  z3::expr bit = z3.get_fresh_bool();
  wr_ord_map.insert( std::make_pair( pr, bit) );
  return bit;
}

z3::expr barrier_synthesis::get_barr_bit( hb_enc::se_ptr e ) {
  return barrier_map.at( e );
}

z3::expr barrier_synthesis::get_lw_barr_bit( hb_enc::se_ptr e ) {
  return light_barrier_map.at( e );
}


z3::expr barrier_synthesis::mk_edge_constraint( hb_enc::se_ptr& b,
                                                hb_enc::se_ptr& a,
                                                z3::expr& hard ) {
  mm_t mm = program->get_mm();
  hb_enc::se_tord_set pending;
  hb_enc::se_vec found;
  std::map< std::tuple<hb_enc::se_ptr,hb_enc::se_ptr,hb_enc::se_ptr>,z3::expr >
    history_map;
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
          z3::expr h_bkk = cssa::wmm_event_cons::check_ppo(mm,k,j) ?
            get_h_var_bit(b,k,k):z3.mk_false();
          // todo: check for the lwsync requirement
          z3::expr lw_k = z3.mk_false();
          if( enable_lw_fences ) { // disable 
            z3::expr lw_k = j->is_wr()? get_lw_barr_bit( k ) : z3.mk_false();
            lw_k = lw_k && get_write_order_bit( b, k );
          }
          z3::expr b_k = get_barr_bit( k );
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
      z3::expr s_i = get_barr_bit( i );
      z3::expr lw_i = get_lw_barr_bit( i );
      hard = hard && implies( s_i, lw_i );
    }
  }
  return get_h_var_bit( b, a, a );

  assert( false );
  return z3.mk_false(); //todo : may be reachable for unreachable pairs
}

void barrier_synthesis::gen_max_sat_problem_new() {
  z3::context& z3_ctx = sol_bad->ctx();

  for( unsigned t = 0; t < program->size(); t++ ) {
     const auto& thread = program->get_thread( t );
     for( const auto& e : thread.events ) {
       // todo : optimize number of variables introduced
       z3::expr m_bit = z3.get_fresh_bool(e->name());
       barrier_map.insert({ e, m_bit });
       soft.push_back( !m_bit );
       z3::expr s_bit = z3.get_fresh_bool("lw_"+e->name());
       light_barrier_map.insert({ e, s_bit });
       soft.push_back( !s_bit );
     }
  }

  z3::expr hard = z3.mk_true();
  for( auto& cycles : all_cycles ) {
    if( cycles.size() == 0 ) continue; // throw error unfixable situation
    z3::expr c_disjunct = z3_ctx.bool_val(false);
    for( auto& cycle : cycles ) {
      z3::expr r_conj = z3.mk_true();
      for( auto edge : cycle.edges ) {
        if( edge.type == edge_type::rpo ) {
          r_conj = r_conj && mk_edge_constraint( edge.before, edge.after, hard ); 
        }
      }
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

    for( auto it=barrier_map.begin(); it != barrier_map.end(); it++ ) {
      hb_enc::se_ptr e = it->first;
      z3::expr b = it->second;
      if( m.eval(b).get_bool() ) {
        barrier_where.push_back( e );
      }else{
        z3::expr b_s = light_barrier_map.at(e);
        if( m.eval(b_s).get_bool() ) {
          soft_barrier_where.push_back( e );
        }
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
