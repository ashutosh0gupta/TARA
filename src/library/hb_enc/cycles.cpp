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

#include "hb_enc/cycles.h"

namespace tara {
namespace hb_enc {

cycle_edge::cycle_edge( hb_enc::se_ptr _before, hb_enc::se_ptr _after, edge_type _type )
    :before(_before),after(_after),type(_type) {}


cycle::cycle( cycle& _c, unsigned i ) {
  edges = _c.edges;
  relaxed_edges = _c.relaxed_edges;
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

void cycle::remove_prefix( hb_enc::se_ptr e ) {
  unsigned i = 0;
  for(; i < edges.size(); i++ ) {
    if( edges[i].before == e ) break;
  }
  remove_prefix(i);
}

void cycle::remove_suffix( hb_enc::se_ptr e ) {
  unsigned i = 0;
  for(; i < edges.size(); i++ ) {
    if( edges[i].after == e ) break;
  }
  remove_suffix(i+1);
}

unsigned cycle::has_cycle() {
  unsigned i = 0;
  hb_enc::se_ptr e = last();
  for(; i < edges.size(); i++ ) {
    if( edges[i].before == e ) break;
  }
  return i;
}

void cycle::pop() {
  auto& edge = edges.back();
  if( edge.type ==  edge_type::rpo ) {
    assert( !relaxed_edges.empty() );
    relaxed_edges.pop_back();
  }
  edges.pop_back();
}

void cycle::clear() {
  edges.clear();
  closed = false;
}

void cycle::close() {
  hb_enc::se_ptr l = last();
  remove_prefix( l );
  if( first() == l ) closed = true;
}

bool cycle::add_edge( hb_enc::se_ptr before, hb_enc::se_ptr after, edge_type t ) {
  if( !closed && last() == before ) {
    cycle_edge ed(before, after, t);
    edges.push_back(ed);
    if( t == edge_type::rpo ) {
      relaxed_edges.push_back( ed );
    }
  }else{
    // throw error
    assert(false);
  }
  // close();
  return closed;
}

bool cycle::add_edge( hb_enc::se_ptr after, edge_type t ) {
  return add_edge( last(), after, t );
}

bool cycle::has_relaxed( cycle_edge& edge) {
  auto it = std::find( relaxed_edges.begin(), relaxed_edges.end(), edge);
  return it != relaxed_edges.end();
}

//I am dominated
bool cycle::is_dominated_by( cycle& c ) {
  for( auto& edge : c.relaxed_edges  ) {
    if( !has_relaxed( edge ) ) return false;
  }
  //all relaxed edges are in c
  // std::cerr << *this << " is dominated by\n"<< c << "\n";
  return true;
}

  std::ostream& operator<<( std::ostream& stream, const cycle& c ) {
  stream << c.name << "(";
  bool first = true;
  for( const auto& edge : c.edges ) {
    if(first) {
      if( edge.before )
        stream << edge.before->name();
      else
        stream << "null";
      first = false;
    }
    switch( edge.type ) {
    case edge_type::hb:  {stream << "->";} break;
    case edge_type::ppo: {stream << "=>";} break;
    case edge_type::rpo: {stream << "~~>";} break;
    }
    if( edge.after )
      stream << edge.after->name();
    else
      stream << "null";
  }
  stream << ")";
  return stream;
}

  void cycles::insert_event( std::vector<hb_enc::se_vec>& event_lists,
                             hb_enc::se_ptr e ) {
  unsigned i_no = e->get_instr_no();

  hb_enc::se_vec& es = event_lists[e->tid];
  auto it = es.begin();
  for(; it < es.end() ;it++) {
    hb_enc::se_ptr& e1 = *it;
    if( e1 == e ) return;
    if( e1->get_instr_no() > i_no ||
        ( e1->get_instr_no() == i_no && e1->is_wr() && e->is_rd() ) ) {
      break;
    }
  }
  es.insert( it, e);
}

void cycles::succ( hb_enc::se_ptr e,
                   const hb_enc::se_set& filter,
                   std::vector< std::pair< hb_enc::se_ptr, edge_type> >& next_set ) {
  for( auto it : hbs ) {
    // auto hb_from_b = *it;
    if( it.first == e ) {
      if( filter.find( it.second ) == filter.end() ) continue;
      next_set.push_back( {it.second,edge_type::hb} );
    }
  }
  const hb_enc::depends_set after = program->may_after.at(e);
  for(std::set<hb_enc::depends>::iterator it = after.begin(); it != after.end(); it++) {
    hb_enc::depends dep = *it;
    z3::expr cond = dep.cond;
    if( filter.find( dep.e ) == filter.end() ) continue;
    if( z3.is_true( dep.cond )  )
      next_set.push_back( {dep.e, edge_type::ppo } );
    else
      next_set.push_back( {dep.e, edge_type::rpo } );
  }
    // break; // todo <- do we miss anything
}


void cycles::find_sccs_rec( hb_enc::se_ptr e,
                            const hb_enc::se_set& filter,
                            std::vector< hb_enc::se_set >& sccs ) {
  index_map[e] = scc_index;
  lowlink_map[e] = scc_index;
  on_stack[e] = true;
  scc_index = scc_index + 1;
  scc_stack.push_back(e);
  std::vector< std::pair< hb_enc::se_ptr, edge_type> > next_set;
  succ( e, filter, next_set );
  for( auto& ep_pair :  next_set ) {
    hb_enc::se_ptr ep = ep_pair.first;
    if( index_map.find(ep) == index_map.end() ) {
      find_sccs_rec( ep, filter, sccs );
      lowlink_map[e] = std::min( lowlink_map.at(e), lowlink_map.at(ep) );
    }else if( on_stack.at(ep) ) {
      lowlink_map.at(e) = std::min( lowlink_map.at(e), index_map.at(ep) );
    }
  }
  if( lowlink_map.at(e) == index_map.at(e) ) {
    // pop to collect its members
    hb_enc::se_set scc;
    hb_enc::se_ptr ep;
    do{
      ep = scc_stack.back();
      scc_stack.pop_back();
      on_stack.at(ep) = false;
      scc.insert( ep );
    }while( ep != e);
    if( scc.size() > 1 ) sccs.push_back( scc );
  }
}

void cycles::find_sccs( const hb_enc::se_set& filter,
                        std::vector< hb_enc::se_set >& sccs ) {
  index_map.clear();
  lowlink_map.clear();
  on_stack.clear();
  scc_index = 0;
  //assert( hbs.size() > 0 );
  while(1) {
    hb_enc::se_ptr e;
    for( const auto hb : hbs ){
      if( index_map.find( hb.first ) == index_map.end() ) { e = hb.first; break; }
      if( index_map.find( hb.second ) == index_map.end() ) { e = hb.second; break; }
    }
    if( e ) {
      find_sccs_rec( e, filter, sccs );
    }else{
      break;
    }
  }

  // if( verbose ) {
  //   auto& stream = std::cout;
  //   stream << "scc detected:\n";
  //   for( auto& scc : sccs ) {
  //     for( auto e : scc )
  //       stream << e->name() << " ";
  //     stream << endl;
  //   }
  // }
}


void cycles::cycles_unblock( hb_enc::se_ptr e ) {
  blocked[e] = false;
  hb_enc::se_set e_set = B_map.at(e);
  B_map.at(e).clear();
  for( hb_enc::se_ptr ep : e_set ) {
    if( blocked.at( ep) )
      cycles_unblock(ep);
  }
}

bool cycles::is_relaxed_dominated( cycle& c , std::vector<cycle>& cs ) {
  for( cycle& cp : cs ) {
    if( c.is_dominated_by(cp) ) {
      return true;
    }
  }
  return false;
}

bool cycles::find_true_cycles_rec( hb_enc::se_ptr e, const hb_enc::se_set& scc,
                                   std::vector<cycle>& found_cycles ) {
  bool f = false;
  blocked[e] = true;
  std::vector< std::pair< hb_enc::se_ptr, edge_type> > next_set;
  succ( e, //hbs, event_lists,
        scc, next_set );
  for( auto& ep_pair :  next_set ) {
    hb_enc::se_ptr ep = ep_pair.first;
    if( ep == root ) {
      ancestor_stack.add_edge( ep, ep_pair.second );
      if( ep_pair.second != edge_type::rpo ||
          !is_relaxed_dominated( ancestor_stack , found_cycles ) ) {
        cycle c( ancestor_stack, ancestor_stack.has_cycle()); // 0 or 1??
        for( auto it = found_cycles.begin(); it != found_cycles.end();) {
          if( it->is_dominated_by( c ) ) {
            it = found_cycles.erase( it );
          }else{
            it++;
          }
        }
        found_cycles.push_back(c);
      }
      ancestor_stack.pop();
      f = true;
    }else if( !blocked[ep] ) {
      ancestor_stack.add_edge( ep, ep_pair.second );
      if( ep_pair.second != edge_type::rpo ||
          !is_relaxed_dominated( ancestor_stack , found_cycles ) ) {
        bool fp = find_true_cycles_rec( ep, //hbs, event_lists,
                                        scc, found_cycles );
        if( fp ) f = true;
      }
    }
  }
  if( f ) {
    cycles_unblock( e );
  }else{
    for( auto& ep_pair :  next_set ) {
      hb_enc::se_ptr ep = ep_pair.first;
      // assert( B_map[ep].empty() )
      B_map[ep].insert(e);
    }
  }
  ancestor_stack.pop();
  return f;
}

void cycles::find_true_cycles( hb_enc::se_ptr e, const hb_enc::se_set& scc,
                               std::vector<cycle>& found_cycles ) {
  ancestor_stack.clear();
  ancestor_stack.add_edge(e,edge_type::hb);
  root = e;
  B_map.clear();
  blocked.clear();
  for( hb_enc::se_ptr ep : scc ) {
    B_map[ep].clear();
    blocked[ep] = false;
  }
  find_true_cycles_rec( e, scc, found_cycles );
}

  void cycles::find_cycles( const hb_enc::hb_vec& c,
                          std::vector<cycle>& found_cycles) {
  hb_enc::se_set all_events;
  hbs.clear();
  event_lists.clear();
  event_lists.resize( program->size() );

  for( const auto& h : c ) {
    hb_enc::se_ptr b = h->e1;
    hb_enc::se_ptr a = h->e2;
    hbs.push_back({b,a});
    insert_event( event_lists, b);
    insert_event( event_lists, a);
    all_events.insert( b );
    all_events.insert( a );
  }

  std::vector< hb_enc::se_set > sccs;
  find_sccs( all_events, sccs );

  while( !sccs.empty() ) {
    auto scc = sccs.back();
    sccs.pop_back();
    if( scc.size() > 1 ) {
      hb_enc::se_ptr e = *scc.begin();
      find_true_cycles( e, scc, found_cycles );
      scc.erase(e);
      find_sccs( scc, sccs );
    }
  }

}

  void cycles::find_cycles( const std::list<hb>& c,std::vector<cycle>& found_cycles) {
    hb_enc::hb_vec vec_c;
    for( auto& h : c ) {
      hb_enc::hb_ptr h_ptr = std::make_shared<hb_enc::hb>(h);
      vec_c.push_back( h_ptr );
    }
    find_cycles( vec_c, found_cycles );
  }

}}
