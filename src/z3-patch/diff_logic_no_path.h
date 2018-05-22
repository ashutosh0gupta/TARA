#ifndef DIFF_LOGIC_NO_PATH_H_
#define DIFF_LOGIC_NO_PATH_H_

#include "smt/diff_logic.h"

// Each node
//  - 0 == "unreachable",
//  - 1 == "forward blocked reachable"
//  - 2 == "both ways blcoked reachable"
//  - 3 == "reachable"

enum reach_mark {
  UNREACH = 0,
  FWD_BLOCKED_REACH = 1,
  REV_BLOCKED_REACH = 2,
  REACH = 3
};

template<class Ext>
template<typename Functor>
bool
dl_graph<Ext>::
no_path( dl_var source, dl_var target, unsigned timestamp, Functor & f ) {
  svector<reach_mark>     bfs_mark;
  bfs_mark.resize(m_assignment.size(), UNREACH);

  svector<bfs_elem> bfs_todo;
  bfs_todo.push_back(bfs_elem(source, -1, null_edge_id));
  bfs_mark[source] = REACH;

  unsigned  m_head = 0;
  numeral gamma;
  while (m_head < bfs_todo.size()) {
    bfs_elem & curr = bfs_todo[m_head];
    int parent_idx  = m_head;
    m_head++;
    dl_var  v = curr.m_var;
    auto curr_src_mark = bfs_mark[v];
    TRACE("dl_bfs", tout << "processing: " << v << "\n";);
    edge_id_vector & edges = m_out_edges[v];
    typename edge_id_vector::iterator it  = edges.begin();
    typename edge_id_vector::iterator end = edges.end();
    for (; it != end; ++it) {
      edge_id e_id = *it;
      edge & e     = m_edges[e_id];
      SASSERT(e.get_source() == v);
      auto e_enable = e.is_enabled();
      set_gamma(e, gamma);
      TRACE( "dl_bfs", tout << "processing edge: ";
             display_edge(tout, e); tout << "gamma: " << gamma << "\n";);
      if( (gamma.is_zero() || (gamma.is_neg()))
           && e.get_timestamp() < timestamp ) {
        dl_var curr_target = e.get_target();
        TRACE("dl_bfs", tout << "curr_target: " << curr_target << 
              ", mark: " << static_cast<int>(bfs_mark[curr_target]) << "\n";);
        char curr_target_mark = bfs_mark[curr_target];
        if( curr_target_mark == UNREACH ) {
          // curr_target never seen before
          bfs_todo.push_back(bfs_elem(curr_target, parent_idx, e_id));
          bfs_mark[curr_target] = e_enable ?
            curr_src_mark : FWD_BLOCKED_REACH;
        }else if( curr_target_mark < REACH && e_enable
                  && curr_src_mark == REACH ) {
          // curr_traget seen blocked and now reachable
          bfs_todo.push_back(bfs_elem(curr_target, parent_idx, e_id));
          bfs_mark[curr_target] = REACH;
        }
        if(curr_target == target && bfs_mark[curr_target] == REACH ) {
          // Reachable path is found
          TRACE("dl_bfs", tout << "found path\n";);
          return false;
        }else{
          // mark that it is reachable backwards
          bfs_mark[curr_target] = REV_BLOCKED_REACH;
        }
      }
    }
  }

  if( bfs_mark[target] == UNREACH ) {
    // empty explanation; simply false
    return true;
  }
  // make explanation
  svector<bfs_elem> rev_bfs_todo;
  rev_bfs_todo.push_back(bfs_elem(target, -1, null_edge_id));
  bfs_mark[target] = REV_BLOCKED_REACH;

  m_head = 0;
  while( m_head < rev_bfs_todo.size() ) {
    bfs_elem & curr_target = rev_bfs_todo[m_head];
    int parent_idx  = m_head;
    m_head++;
    dl_var  v = curr_target.m_var;
    TRACE("dl_bfs", tout << "processing: " << v << "\n";);
    edge_id_vector & edges = m_in_edges[v];
    typename edge_id_vector::iterator it  = edges.begin();
    typename edge_id_vector::iterator end = edges.end();
    for (; it != end; ++it) {
      edge_id e_id = *it;
      edge & e     = m_edges[e_id];
      SASSERT(e.get_target() == v);
      // auto e_enable = e.is_enabled();
      TRACE( "dl_bfs", tout << "processing edge: "; display_edge(tout, e););
      dl_var curr_source = e.get_source();
      char curr_source_mark = bfs_mark[curr_source];
      TRACE("dl_bfs", tout << "curr_source: " << curr_source << 
            ", mark: " << static_cast<int>(curr_source_mark) << "\n";);
      if( curr_source_mark == REACH ) {
        SASSERT( !e.is_enabled() );
        // e is at the boundry of REACH and REV_BLOCKED_REACH
        f(e.get_explanation());
      }else if( curr_source_mark == FWD_BLOCKED_REACH ) {
        // curr_traget seen blocked and now reachable
        rev_bfs_todo.push_back(bfs_elem(curr_source, parent_idx, e_id));
        bfs_mark[curr_source] = REV_BLOCKED_REACH;
      }
    }
  }
  return true;
}

template<class Ext>
template<typename Functor>
bool
dl_graph<Ext>::
no_path_both_way( dl_var source, dl_var target, unsigned timestamp, Functor & f ) {
  return no_path( source, target, timestamp, f ) &&
    no_path(  target, source , timestamp, f );
}


#endif /* DIFF_LOGIC_NO_PATH_H_ */
