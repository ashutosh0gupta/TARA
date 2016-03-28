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

    // cycles.clear();
    // do the synthesis
    // locks.clear();
    // wait_notifies.clear();
    // barriers.clear();
    // cycle_counter = 1;
    // barrier_counter = 1;

    // measure time
    auto start_time = chrono::steady_clock::now();

    find_cycles( bad_dnf );
    
    // Management
    // info = to_string(cycles.size()) + " " //+
    //   // to_string(barriers.size()) + " " 
    //   ;
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
  print_all_cycles( stream );

  stream << "barrier synthesis printing code is not written!!\n";
  // print locks and wait-notifies
  // if (!machine_readable) stream << "Locks: ";
  // for (auto l:locks) {
  //   stream << l << " ";
  // }
  // stream << endl;
  // if (!machine_readable) stream << "Barriers: ";
  // for (auto b:barriers) {
  //   stream << b << " ";
  // }
  // stream << endl;
  // if (!machine_readable) stream << "Wait-notifies: ";
  // for (auto hb:wait_notifies) {
  //   stream << hb << " ";
  // }
  stream << endl;
}

void barrier_synthesis::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  metric.additional_info = info;
}


//----------------------------------------------------------------------------
void barrier_synthesis::gen_max_sat_problem() {
  
}

