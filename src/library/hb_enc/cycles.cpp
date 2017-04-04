/*
 * Copyright 2017, TIFR, India
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

  bool is_weak_edge( edge_type t ) {
    return t ==  edge_type::rpo || t == edge_type::fr || t == edge_type::rf;
  };

cycle_edge::cycle_edge( hb_enc::se_ptr _before,
                        hb_enc::se_ptr _after,
                        edge_type _type )
  : before( _before )
  , after( _after )
  , type( _type ) {}


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
  auto& t = edge.type;
  if( is_weak_edge( t ) ) {
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
  assert( t != edge_type::rpo || before->tid == after->tid );
  assert( t != edge_type::ppo || before->tid == after->tid );
  assert( before == NULL || t != edge_type::hb  || before->tid != after->tid );
  assert( t != edge_type::fr  || before->tid != after->tid );
  assert( t != edge_type::rf  || before->tid != after->tid );

  if( !closed && last() == before ) {
    cycle_edge ed(before, after, t);
    edges.push_back(ed);
    if( is_weak_edge( t ) ) {
      relaxed_edges.push_back( ed );
    }
  }else{
    hb_enc_error( "Mismatch in extending the cycle!!" );
  }
  // close();
  return closed;
}

bool cycle::add_edge( hb_enc::se_ptr after, edge_type t ) {
  return add_edge( last(), after, t );
}

bool cycle::has_relaxed( cycle_edge& edge) const {
  auto it = std::find( relaxed_edges.begin(), relaxed_edges.end(), edge);
  return it != relaxed_edges.end();
}

//I am dominated
bool cycle::is_dominated_by( cycle& c ) const {
  for( auto& edge : c.relaxed_edges  ) {
    if( !has_relaxed( edge ) ) return false;
  }
  //all relaxed edges are in c
  // std::cerr << *this << " is dominated by\n"<< c << "\n";
  return true;
}

  void debug_print( std::ostream& stream,
                    const std::vector<cycle_edge>& eds) {
    bool first = true;
    for( const auto& edge : eds ) {
      if(first) {
        if( edge.before )
          stream << edge.before->name();
        else
          stream << "null";
        first = false;
      }
      switch( edge.type ) {
      case hb_enc::edge_type::hb:  { stream << "->";  } break;
      case hb_enc::edge_type::ppo: { stream << "=>";  } break;
      case hb_enc::edge_type::rpo: { stream << "~~>"; } break;
      case hb_enc::edge_type::rf:  { stream << "==>"; } break;
      case hb_enc::edge_type::fr:  { stream << "~~~>";} break;
      default:
        hb_enc_error( "uncover case!!" );
      }
      if( edge.after )
        stream << edge.after->name();
      else
        stream << "null";
    }
  }

  void debug_print( std::ostream& stream, const cycle_edge& ed ) {
    std::vector<cycle_edge> eds;
    eds.push_back( ed );
    debug_print( stream, eds );
  }

  std::ostream& operator<<( std::ostream& stream, const cycle& c ) {
    stream << c.name << "(";
    debug_print( stream, c.edges );
    stream << ")";
    return stream;
  }

void cycles::succ( hb_enc::se_ptr e,
                   const hb_enc::se_set& filter,
                   std::vector< std::pair< hb_enc::se_ptr, edge_type> >& next_set ) {

  for( auto ed : hbs ) {
    if( ed.before == e ) {
      if( filter.find( ed.after ) == filter.end() ) continue;
      next_set.push_back( {ed.after, ed.type } );
      assert( ed.type != edge_type::ppo && ed.type != edge_type::rpo );
    }
  }

  for( auto& ed : learned_fr_edges ) {
    if( ed.before == e ) {
      if( !helpers::exists( filter, ed.after ) ) continue;
      next_set.push_back( {ed.after, edge_type::fr } );
    }
  }

  if( e->is_pre() || e->is_post() ) return;
  //todo: inefficient code... needs a fix
  for( auto& ep: program->get_thread(e->tid).events ) {
    if( filter.find( ep ) == filter.end() ) continue;
    for( const hb_enc::depends& dep : program->ppo_before.at(ep) ) {
      if( dep.e != e ) continue;
      assert( e->tid == ep->tid );
      if( program->is_mm_c11() ) { //todo: c11 case should be handelled properly
        next_set.push_back( {ep, edge_type::ppo } );
      }else{
        if( z3.is_true( dep.cond )  )
          next_set.push_back( {ep, edge_type::ppo } );
        else
          next_set.push_back( {ep, edge_type::rpo } );
      }
    }
  }

  // const hb_enc::depends_set& after = program->may_after.at(e);
  // for( const hb_enc::depends& dep : after ) {
  //   z3::expr cond = dep.cond;
  //   if( filter.find( dep.e ) == filter.end() ) continue;
  //   if( z3.is_true( dep.cond )  )
  //     next_set.push_back( {dep.e, edge_type::ppo } );
  //   else
  //     next_set.push_back( {dep.e, edge_type::rpo } );
  // }
    // break; // todo <- do we miss anything
  // for( auto& ep_pair :  next_set ) {
  //   std::cerr << "\n" << e->name() << "-->\n";
  //   if(ep_pair.second == edge_type::rpo )
  //     std::cerr << ep_pair.first->name() << "\n";
  // }

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
  while(1) {
    hb_enc::se_ptr e;
    for( const auto hb : hbs ) {
      if( index_map.find( hb.before) == index_map.end()) { e = hb.before; break;}
      if( index_map.find( hb.after ) == index_map.end()) { e = hb.after;  break;}
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

bool cycles::is_relaxed_dominated( const cycle& c , std::vector<cycle>& cs ) {
  for( cycle& cp : cs ) {
    if( c.is_dominated_by(cp) ) {
      return true;
    }
  }
  return false;
}

bool cycles::find_true_cycles_rec( hb_enc::se_ptr e,
                                   const hb_enc::se_set& scc,
                                   std::vector<cycle>& found_cycles ) {
  assert( ancestor_stack.last() == e );
  // std::cout << "e:"<<ancestor_stack << "\n";

  bool f = false;
  blocked[e] = true;
  std::vector< std::pair< hb_enc::se_ptr, edge_type> > next_set;
  succ( e, scc, next_set );
  for( auto& ep_pair :  next_set ) {
    hb_enc::se_ptr ep = ep_pair.first;
    if( ep == root ) {
      ancestor_stack.add_edge( e, ep, ep_pair.second );
      if( !is_weak_edge(ep_pair.second ) ||
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
      ancestor_stack.add_edge( e, ep, ep_pair.second );
      if( !is_weak_edge(ep_pair.second ) ||
          !is_relaxed_dominated( ancestor_stack , found_cycles ) ) {
        bool fp = find_true_cycles_rec( ep, scc, found_cycles );
        assert( ancestor_stack.last() == e );
        if( fp ) f = true;
      }else{
        ancestor_stack.pop();
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
  // std::cout << "r:"<<ancestor_stack << "\n";
  return f;
}

void cycles::find_true_cycles( hb_enc::se_ptr e,
                               const hb_enc::se_set& scc,
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


  void
  cycles::find_fr_edges( const hb_enc::hb_vec& c,
                         const hb_enc::se_set& all_es,
                         std::vector< cycle_edge >& new_edges ) {
    new_edges.clear();
    hb_enc::hb_vec hbs_local;
    hb_enc::hb_vec rfs;
    std::vector< std::pair<hb_enc::se_ptr,hb_enc::se_ptr> > thread_syncs; //quick and dirty: thrd create/join orderings
    for( const auto& h : c ) {
      if( h->type == hb_t::phb && !h->is_neg ) {
        hbs_local.push_back( h );
      }
      if( h->type == hb_t::rf ) {
        assert( !h->is_neg );
        rfs.push_back( h );
      }
    }

    hb_enc::se_set all_es_copy = all_es;

    for( unsigned i = 0; i < program->size(); i ++ ) {
      auto& ce = program->get_create_event(i);
      auto& je = program->get_join_event(i);
      auto& se = program->get_thread(i).start_event;
      auto& fe = program->get_thread(i).final_event;
      all_es_copy.insert( je );
      all_es_copy.insert( ce );
      all_es_copy.insert( se );
      all_es_copy.insert( fe );
      thread_syncs.push_back({ce,se});
      thread_syncs.push_back({fe,je});
    }

    hb_enc::se_vec stack,es;
    helpers::set_to_vec( all_es_copy, stack );
    es = stack;

    std::map< se_ptr, unsigned > order_map;
    while( !stack.empty() ) {
      se_ptr e = stack.back(); //std::cerr << e->name() << "\n";
      if( helpers::exists( order_map, e ) ) {
        stack.pop_back();
        continue;
      }
      auto& prevs = program->seq_before.at(e);
      bool ready = true;
      unsigned m = 0;
      for( auto& ep : prevs ) {
        if( !helpers::exists( es, ep ) ) continue;
        if( !helpers::exists( order_map, ep ) ) {
          ready = false;
          stack.push_back( ep );
        }else{
          if( ready == true && m < order_map.at(ep) ) m = order_map.at(ep);
        }
      }
      for( const auto& h : hbs_local ) {
        if( h->e2 != e ) continue;
        if( !helpers::exists( order_map, h->e1 ) ) {
          ready = false;
          stack.push_back( h->e1 );
        }else{
          if( ready == true && m < order_map.at(h->e1)) m = order_map.at(h->e1);
        }
      }
      for( const auto& sync : thread_syncs ) {
        //quick and dirty, prevs should have already included everything
        if( sync.second != e ) continue;
        auto& e1 = sync.first;
        if( !helpers::exists( order_map, e1 ) ) {
          ready = false;
          stack.push_back( e1 );
        }else{
          if( ready == true && m < order_map.at(e1) ) m = order_map.at(e1);
        }
      }
      if( ready == true ) {
        order_map[e] = m + 1;
      }
    }

    std::sort( es.begin(), es.end(),
               [&](const se_ptr& x, const se_ptr& y) {
                 return order_map.at(x) < order_map.at(y);
               } );

    hb_enc::se_to_ses_map pasts;
    for( auto e : es ) {
      auto& prevs = program->seq_before.at(e);
      pasts[e] = prevs;
      hb_enc::se_set& new_prevs = pasts.at(e);
      for( auto epp : prevs ) {
        if( helpers::exists( all_es_copy, epp ) ) {
          helpers::set_insert( new_prevs, pasts.at(epp) );
        }
      }
      for( const auto& h : hbs_local ) {
        if( h->e2 == e ) {
          new_prevs.insert( h->e1 );
          helpers::set_insert( new_prevs, pasts.at(h->e1) );
        }
      }
      for( const auto& sync : thread_syncs ) {
        //quick and dirty, prevs should have already included everything
        if( sync.second != e ) continue;
        new_prevs.insert( sync.first );
        helpers::set_insert( new_prevs, pasts.at(sync.first) );
      }
    }

    hb_enc::se_to_ses_map posts;
    for (auto rit = es.rbegin(); rit!= es.rend(); ++rit) {
      hb_enc::se_ptr e = *rit;
      auto& nexts = program->seq_after.at(e);
      posts[e] = nexts;
      hb_enc::se_set& new_nexts = posts.at(e);
      for( auto epp : nexts ) {
        if( helpers::exists( all_es_copy, epp ) ) {
          helpers::set_insert( new_nexts, posts.at(epp) );
        }
      }
      for( const auto& h : hbs_local ) {
        if( h->e1 == e ) {
          helpers::set_insert( new_nexts, posts.at(h->e2) );
        }
      }
      for( const auto& sync : thread_syncs ) {
        //quick and dirty, prevs should have already included everything
        if( sync.first != e ) continue;
        new_nexts.insert( sync.second );
        helpers::set_insert( new_nexts, posts.at(sync.second) );
      }
      // debug_print( std::cerr, pasts );
    }
    // debug_print( std::cerr, posts );

    for( const auto& h : rfs ) {
      hb_enc::se_ptr w = h->e1;
      hb_enc::se_ptr r = h->e2;
      for( auto wp : pasts.at(w) ) {
        if( helpers::exists( all_es, wp ) &&
            ( wp->is_pre() || ( wp->is_wr() && w->access_same_var(wp) )) ) {
          cycle_edge ed( wp, r, edge_type::fr );
          new_edges.push_back(ed);
          for( const auto& h : rfs ) {
            if( wp == h->e1 && r->access_same_var( h->e2 ) ) {
              cycle_edge ed( h->e2, r, edge_type::fr );
              new_edges.push_back(ed);
            }
          }
        }
      }
      for( auto wp : posts.at(w) ) {
        if( wp->is_wr() && w->access_same_var(wp) &&
            helpers::exists( all_es, wp ) ) {
          cycle_edge ed( r, wp, edge_type::fr );
          new_edges.push_back(ed);
        }
      }
      for( auto rp : pasts.at(r) ) {
        if( helpers::exists( all_es, rp ) &&
            ( rp->is_rd() && r->access_same_var(rp) ) ) {
          for( const auto& h : rfs ) {
            if( rp == h->e2 && r->access_same_var( h->e2 ) ) {
              cycle_edge ed( h->e1, w, edge_type::fr );
              new_edges.push_back(ed);
            }
          }
        }
      }
    }
    if( verbose > 1 ) {
      for( auto& ed : new_edges ) {
        debug_print( std::cerr, ed ); std::cerr << "\n";
        // std::cerr << ed.before->name() << "->" << ed.after->name() << "\n";
      }
    }
  }


  void cycles::find_cycles( const hb_enc::hb_vec& c,
                          std::vector<cycle>& found_cycles) {
  hb_enc::se_set all_events;
  hbs.clear();
  // event_lists.clear();
  // event_lists.resize( program->size() );

  for( const auto& h : c ) {
    hb_enc::se_ptr before = h->e1;
    hb_enc::se_ptr after = h->e2;
    if( h->is_neg && h->type == hb_t::phb ) {
      cycle_edge ed(before, after, edge_type::fr);
      hbs.push_back(ed);
    }else if( h->type == hb_t::rf ) {
      auto rt = h->e1->is_rlx() || h->e2->is_rlx()? edge_type::rf : edge_type::hb;
      cycle_edge ed(before, after, rt );
      hbs.push_back(ed);
    }else{
      cycle_edge ed(before, after, edge_type::hb);
      hbs.push_back(ed);
    }
    all_events.insert( before );
    all_events.insert( after  );
    // hb_enc::se_ptr b = h->e1;
    // hb_enc::se_ptr a = h->e2;
    // hbs.push_back({b,a});
    // insert_event( event_lists, b);
    // insert_event( event_lists, a);
    // all_events.insert( b );
    // all_events.insert( a );
  }

  if( program->is_mm_c11() )
    find_fr_edges( c, all_events, learned_fr_edges );

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

  void cycles::find_cycles( const std::list<hb>& c,
                            std::vector<cycle>& found_cycles ) {
    hb_enc::hb_vec vec_c;
    for( auto& h : c ) {
      hb_enc::hb_ptr h_ptr = std::make_shared<hb_enc::hb>(h);
      vec_c.push_back( h_ptr );
    }
    find_cycles( vec_c, found_cycles );
  }

}}
