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


#include "barrier_synthesis.h"
#include "api/output/output_base_utilities.h"
#include <chrono>
#include <algorithm>

using namespace tara;
using namespace tara::cssa;
using namespace tara::api::output;
using namespace tara::helpers;
using namespace std;

namespace tara {
namespace api {
namespace output {

ostream& operator<<( ostream& stream, const cycle& c ) {
  stream << c.name << "(";
  bool first = true;
  for( const auto& edge : c.edges ) {
    if(first) {
      stream << edge.before->name();
      first = false;
    }
    switch( edge.type ) {
    case edge_type::hb:  {stream << "->";} break;
    case edge_type::ppo: {stream << "=>";} break;
    case edge_type::rpo: {stream << "~~>";} break;
    }
    stream << edge.after->name();
  }
  stream << ")";
  return stream;
}

// ostream& operator<<(ostream& stream, const barrier& barrier)
// {
//   // stream << barrier.name << "(";
//   // for(auto l = barrier.locations.begin(); l!=barrier.locations.end(); l++) {
//   //   stream << *l ;
//   //   if (!last_element(l, barrier.locations)) stream << ", ";
//   // }
//   // stream << ")";
//   // return stream;
// }

}}}

cycle_edge::cycle_edge( se_ptr _before, se_ptr _after, edge_type _type )
    :before(_before),after(_after),type(_type) {}

bool ValueComp(const se_ptr & before,const se_ptr & after)
{
	return before->e_v->instr_no < after->e_v->instr_no;
}

cycle::cycle( cycle& _c, unsigned i ) {
  edges = _c.edges;
  remove_prefix(i);
  assert( first() == last() );
}

void cycle::remove_prefix( unsigned i ) {
  assert( i <= edges.size() );
  edges.erase( edges.begin(), edges.begin()+i );
}

void cycle::remove_suffix( unsigned i ) {
  assert( i <= edges.size() );
  edges.erase( edges.end()-i, edges.end() );
}

void cycle::remove_prefix( se_ptr e ) {
  unsigned i = 0;
  for(; i < edges.size(); i++ ) {
    if( edges[i].before == e ) break;
  }
  remove_prefix(i);
}

void cycle::remove_suffix( se_ptr e ) {
  unsigned i = 0;
  for(; i < edges.size(); i++ ) {
    if( edges[i].after == e ) break;
  }
  remove_suffix(i+1);
}

unsigned cycle::has_cycle() {
  unsigned i = 0;
  se_ptr e = last();
  for(; i < edges.size(); i++ ) {
    if( edges[i].before == e ) break;
  }
  return i;
}

void cycle::pop() {
  edges.pop_back();
}

void cycle::close() {
  se_ptr l = last();
  remove_prefix( l );
  if( first() == l ) closed = true;
}

bool cycle::add_edge( se_ptr before, se_ptr after, edge_type t ) {
  if( !closed && last() == before ) {
    cycle_edge ed(before, after, t);
    edges.push_back(ed);
  }else{
    // throw error
    assert(false);
  }
  // close();
  return closed;
}

bool cycle::add_edge( se_ptr after, edge_type t ) {
  return add_edge( last(), after, t );
}

//----------------------------------------------------------------------------

barrier_synthesis::barrier_synthesis(bool verify, bool print_nfs)
  : print_nfs(print_nfs)
  , normal_form(false, false, false, true, verify)
{}

void barrier_synthesis::init( const hb_enc::encoding& hb_encoding,
                              const z3::solver& sol_desired,
                              const z3::solver& sol_undesired,
                              std::shared_ptr< const cssa::program > _program )
{
    output_base::init(hb_encoding, sol_desired, sol_undesired, _program);
    normal_form.init(hb_encoding, sol_desired, sol_undesired, _program);
    // program = _program;
}


void barrier_synthesis::output(const z3::expr& output)
{
    output_base::output(output);
    normal_form.output(output);
    
    nf::result_type bad_dnf = normal_form.get_result(true, true);

    // measure time
    auto start_time = chrono::steady_clock::now();

    find_cycles( bad_dnf );

    gen_max_sat_problem();
    {
      std::cout << cut[0] << endl;
      std::cout << "soft:" << endl;
      for( auto s : soft ) {
        std::cout << s << endl;
      }
    }
    z3::model m =fu_malik_maxsat( sol_bad->ctx(), cut[0], soft );
    for( auto it=segment_map.begin(); it != segment_map.end(); it++ ) {
      se_ptr e = it->first;
      z3::expr b = it->second;
      if( m.eval(b).get_bool() )
        barrier_where.push_back( e );
    }

    info = to_string(all_cycles.size()) + " " + to_string(barrier_where.size()) + " ";

    auto delay = chrono::steady_clock::now() - start_time;
    time = chrono::duration_cast<chrono::microseconds>(delay).count();
}

edge_type barrier_synthesis::is_ppo(se_ptr before, se_ptr after) {

  assert( before );
  assert( after );
  assert( before->is_rd() || before->is_wr() );
  assert( after->is_rd() || after->is_wr() );
  assert( before->tid == after->tid );
  if( !program->is_mm_sc() && !program->is_mm_tso() &&
      !program->is_mm_pso() && !program->is_mm_rmo() )
    program->unsupported_mm();
  unsigned b_num = before->get_instr_no();
  unsigned a_num = after->get_instr_no();
  assert( b_num <= a_num);
  if( a_num == b_num ) {
    if( before->is_rd() && after->is_wr() ) return edge_type::ppo;
    return edge_type::rpo;
  }
  if( program->has_barrier_in_range( before->tid, b_num, a_num ) )
    return edge_type::ppo;

  if( program->is_mm_sc() ) {
    return edge_type::ppo;
  }else if( program->is_mm_tso() ) {
    if( before->is_wr() && after->is_rd() ) return edge_type::rpo;
    return edge_type::ppo;
  }else if( program->is_mm_pso() ) {
    if( before->is_rd() ) return edge_type::ppo;
    if( before->is_wr() && after->is_rd() ) return edge_type::rpo;
    if( before->prog_v == after->prog_v ) return edge_type::ppo;
    return edge_type::rpo;
  }else if( program->is_mm_rmo() ) {
    if( after->is_rd() ) return edge_type::rpo;
    if( before->prog_v == after->prog_v ) return edge_type::ppo;
    if( before->is_wr() ) return edge_type::rpo;
    auto& deps = program->dependency_relation.at(after);
    if( deps.find(before) != deps.end() ) return edge_type::ppo;
    return edge_type::rpo;
  }else{
    program->unsupported_mm();
  }
  return edge_type::ppo;
}

void barrier_synthesis::insert_event( vector<se_vec>& event_lists,
                                      se_ptr e ) {
  unsigned i_no = e->get_instr_no();

  se_vec& es = event_lists[e->tid];
  auto it = es.begin();
  for(; it < es.end() ;it++) {
    se_ptr& e1 = *it;
    if( e1 == e ) return;
    if( e1->get_instr_no() > i_no ||
        ( e1->get_instr_no() == i_no && e1->is_wr() && e->is_rd() ) ) {
      break;
    }
  }
  es.insert( it, e);
}

typedef  std::vector< pair<se_ptr,se_ptr> > hb_conj;
// typedef  vector< hb_conj > se_cnf;

void barrier_synthesis::find_cycles(nf::result_type& bad_dnf) {
  all_cycles.clear();
  all_cycles.resize( bad_dnf.size() );
  unsigned bad_dnf_num = 0;
  for( auto c : bad_dnf ) {
    hb_conj hbs;
    vector<se_vec> event_lists;
    event_lists.resize( program->no_of_threads() );

    for( tara::hb_enc::hb& h : c ) {
      se_ptr b = program->se_store.at( h.loc1->name );
      se_ptr a = program->se_store.at( h.loc2->name );
      hbs.push_back({b,a});
      insert_event( event_lists, b);
      insert_event( event_lists, a);
    }
    vector<cycle>& cycles = all_cycles[bad_dnf_num];

    while( !hbs.empty() ) {
      auto hb= hbs[0];
      se_ptr b = hb.first;
      // se_ptr a = hb->second;
      set<se_ptr> explored;
      cycle ancestor; // current candidate cycle
      std::vector< pair<se_ptr,edge_type> > stack;
      stack.push_back({b,edge_type::hb});
      while( !stack.empty() ) {
        pair<se_ptr,edge_type> pair = stack.back();
        se_ptr b = pair.first;
        edge_type type = pair.second;
        if( explored.find(b) != explored.end() ) {
          stack.pop_back();
          // explored
        }else if( ancestor.last() == b ) {
          // subtree has been explored; now I am also explored
          explored.insert(b);
          stack.pop_back();
          ancestor.pop();
        }else{
          ancestor.add_edge(b,type);
          unsigned stem_len = ancestor.has_cycle();
          if( stem_len != ancestor.size() ) {
            //cycle detected
            cycle c(ancestor, stem_len);
            cycles.push_back(c);
          }else{
            // Further expansion
            for( auto it = hbs.begin(); it != hbs.end(); ) {
              auto hb_from_b = *it;
              if( hb_from_b.first == b ) {
                se_ptr a = hb_from_b.second;
                stack.push_back( {a,edge_type::hb} );
                it = hbs.erase(it);
              }else{
                it++;
              }
            }
            se_vec& es = event_lists[b->tid];
            auto it = es.begin();
            for(;it < es.end();it++) {
              if( *it == b ) break;
            }
            for(;it < es.end();it++) {
              se_ptr a = *it;
              if( a->get_instr_no() != b->get_instr_no() ) break;
              if( a->is_wr() && b->is_rd() ) break;
            }
            for(;it < es.end();it++) {
              se_ptr a = *it;
              stack.push_back( {a, is_ppo(b, a) });
            }
          }
        }
      }
    }
    bad_dnf_num++;
  }
  std::cout<<"Number of Cycles\t"<<all_cycles[0].size()<<"\n";
}

void barrier_synthesis::print_all_cycles( ostream& stream ) const
{
  stream << "cycles found!\n";
  for( auto& cycles : all_cycles ) {
    stream << "[" << endl;
    for( auto& cycle : cycles ) {
      stream << cycle << endl;
    }
    stream << "]" << endl;
  }
}

void barrier_synthesis::print(ostream& stream, bool machine_readable) const
{
  if (print_nfs && !machine_readable)
    normal_form.print(stream, false);
  // print_all_cycles( stream );

  stream <<"Barriers must be inserted after these places:- \n";
  for ( unsigned i = 0; i < barrier_where.size(); i++ ) {
    stream << "thread " << barrier_where[i]->get_tid() << ",instr no "
           <<  barrier_where[i]->get_instr_no()  << endl;
  }
  print_all_cycles(stream);
  stream << endl;
}

void barrier_synthesis::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  metric.additional_info = info;
}


//----------------------------------------------------------------------------
z3::expr barrier_synthesis::get_fresh_bool() {
  static unsigned count = 0;
  z3::context& z3_ctx = sol_bad->ctx();
  stringstream b_name;
  b_name<<"b"<<count;
  z3::expr b = z3_ctx.bool_const(b_name.str().c_str());
  count++;
  return b;
}

bool cmp (se_ptr a,se_ptr b) { return (a->get_instr_no() <b->get_instr_no()); }

void barrier_synthesis::gen_max_sat_problem() {
  z3::context& z3_ctx = sol_bad->ctx();

  for( auto& cycles : all_cycles ) {
      for( auto& cycle : cycles ) {
        for( auto edge : cycle.edges ) {
          if( edge.type==edge_type::rpo ) {
            //check each edge for rpo: push them in another vector
            //compare the edges in same thread
            tid_to_se_ptr[ edge.before->tid ].push_back( edge.before );
            tid_to_se_ptr[ edge.before->tid ].push_back( edge.after );
          }
        }
      }
  }

  for( auto it = tid_to_se_ptr.begin();  it != tid_to_se_ptr.end(); it++ ) {
    std::vector<se_ptr>& vec = it->second;
    std::sort( vec.begin(), vec.end(), cmp );
    vec.erase( std::unique( vec.begin(), vec.end() ), vec.end() );
    for(auto e : vec ) {
      z3::expr s = get_fresh_bool();
      segment_map.insert( {e,s} );
      soft.push_back( !s);
    }
  }

  z3::expr hard = z3_ctx.bool_val(true);
  for( auto& cycles : all_cycles ) {
    if( cycles.size() == 0 ) continue;
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
      z3::expr c = get_fresh_bool();
      // z3_to_cycle.insert({c, &cycle});
      c_disjunct=c_disjunct || c;
      hard = hard && implies( c, r_conjunct );
    }
    hard = hard && c_disjunct;
  }
  cut.push_back( hard );

}



// ===========================================================================
// max sat code

void barrier_synthesis:: assert_soft_constraints( z3::solver&s ,z3::context& ctx,
                              std::vector<z3::expr>& cnstrs,
                              std::vector<z3::expr>& aux_vars )
{
  for( auto f : cnstrs ) {
    auto n = get_fresh_bool();
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
    z3::expr curr_xor = (vars[i] != vars[i-1]);
    result = result && (!last_xor || curr_xor);
    last_xor = curr_xor;
  }
  return result;
}

int barrier_synthesis:: fu_malik_maxsat_step( z3::solver &s,
                          z3::context &ctx,
                          std::vector<z3::expr>& soft_cnstrs,
                          std::vector<z3::expr>& aux_vars ) {
    z3::expr_vector assumptions(ctx);
    z3::expr_vector core(ctx);
    for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
      assumptions.push_back(!aux_vars[i]);
    }
    if (s.check(assumptions) != z3::check_result::unsat) {
      return 1; // done
    }else {
      core=s.unsat_core();
      z3::expr_vector block_vars(ctx);
      // update soft-constraints and aux_vars
      for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
        unsigned j;
        // check whether assumption[i] is in the core or not
        for( j = 0; j < core.size(); j++ ) {
          if( assumptions[i] == core[j] )
            break;
        }
        if (j < core.size()) {
          z3::expr block_var = get_fresh_bool();
          z3::expr new_aux_var = get_fresh_bool();
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

z3::model barrier_synthesis:: fu_malik_maxsat( z3::context& ctx,
                                         z3::expr hard,
                                         std::vector<z3::expr>& soft_cnstrs ) {
    unsigned k;
    z3::solver s(ctx);
    s.add( hard );
    // assert_hard_constraints(s, hard_cnstrs);
    // printf("checking whether hard constraints are satisfiable...\n");
    assert( s.check() != z3::unsat );
    assert( soft_cnstrs.size() > 0 );

    std::vector<z3::expr> aux_vars;
    assert_soft_constraints( s,ctx ,soft_cnstrs, aux_vars );
    k = 0;
    for (;;) {
      printf("iteration %d\n", k);
      if( fu_malik_maxsat_step(s, ctx, soft_cnstrs, aux_vars) ) {
    	  z3::model m=s.get_model();
    	  //return soft_cnstrs.size() - k;
    	  return m;
      }
      k++;
    }
}
