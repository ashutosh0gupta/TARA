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

#include "hb_enc/symbolic_event.h"
#include "hb_enc/encoding.h"
#include "hb_enc/hb_enc_exception.h"

using namespace tara;
using namespace tara::hb_enc;
using namespace tara::helpers;

using namespace std;

//----------------------------------------------------------------------------
// utilities for symbolic events


std::shared_ptr<tara::hb_enc::location>
symbolic_event::create_internal_event( helpers::z3interf& z3,
                                       std::string e_name, unsigned tid,
                                       unsigned instr_no, bool special )
{
  auto e_v = make_shared<hb_enc::location>( z3.c, e_name, special );
  e_v->thread = tid;
  e_v->instr_no = instr_no; // not used in post POPL15

  return e_v;
}

void symbolic_event::update_topological_order() {
  unsigned max = 0;
  for( const se_ptr e : prev_events)
    if( max < e->get_topological_order() ) max = e->get_topological_order();
  topological_order = max + 1;
  o_tag = o_tag_t::na;
}

symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events, unsigned instr_no,
                                const cssa::variable& _v,
                                const cssa::variable& _prog_v,
                                std::string _loc, event_t _et )
  : tid(_tid)
  , v(_v)
  , prog_v( _prog_v )
  , loc_name(_loc)
  , et( _et )
  , prev_events( _prev_events )
  , guard(z3.c)
{
  if( et != event_t::r &&  event_t::w != et ) {
    throw hb_enc_exception("symboic event with wrong parameters!");
  }
  bool is_read = (et == event_t::r); // || event_t::f == et);
  std::string et_name = is_read ? "R" : "W";
  std::string e_name = et_name + "#" + v.name;
  e_v = create_internal_event(   z3,            e_name,tid,instr_no,false);
  thin_v = create_internal_event(z3,"__thin__" +e_name,tid,instr_no,false);
  c11_hb_v=create_internal_event(z3,"__hb__"   +e_name,tid,instr_no,false);
  // c11_mo_v=create_internal_event(z3,"__mo__"   +e_name,tid,instr_no,false);
  // c11_sc_v=create_internal_event(z3,"__sc__"   +e_name,tid,instr_no,false);
  update_topological_order();
}


// barrier events
symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events, unsigned instr_no,
                                std::string _loc, event_t _et )
  : tid(_tid)
  , v("dummy",z3.c)
  , prog_v( "dummy",z3.c)
  , loc_name(_loc)
  , et( _et )
  , prev_events( _prev_events )
  , guard(z3.c)
{
  std::string e_name;
  switch( et ) {
  case event_t::barr  : { e_name = "barr#";   break; }
  case event_t::barr_b: { e_name = "barr_b#"; break; }
  case event_t::barr_a: { e_name = "barr_a#"; break; }
  case event_t::pre   : { e_name = "pre#" ;   break; }
  case event_t::post  : { e_name = "post#";   break; }
  default: hb_enc_exception("unreachable code!!");
  }
  e_name = e_name+loc_name;
  e_v = create_internal_event    (z3,            e_name,tid,instr_no,true);
  thin_v = create_internal_event (z3, "__thin__"+e_name,tid,instr_no,true);
  c11_hb_v =create_internal_event(z3, "__hb__"  +e_name,tid,instr_no,true);
  // c11_mo_v =create_internal_event(z3, "__mo__"  +e_name,tid,instr_no,true);
  // c11_sc_v =create_internal_event(z3, "__sc__"  +e_name,tid,instr_no,true);
  // (event_t::post == et), prog_v.name );
  // (event_t::post == et), prog_v.name );
  update_topological_order();
}

z3::expr symbolic_event::get_var_expr( const cssa::variable& g ) {
  assert( et == event_t::r   || et == event_t::w    ||
          et == event_t::pre || et == event_t::post );
  if( et == event_t::r || et == event_t::w ) {
    z3::expr v_expr = (z3::expr)(v);
    return v_expr;
  }
  cssa::variable tmp_v(g.sort.ctx());
  switch( et ) {
  // case event_t::barr: { tmp_v = g+"#barr";  break; }
  case event_t::pre : { tmp_v = g+"#pre" ;  break; }
  case event_t::post: { tmp_v = g+"#post";  break; }
  default: hb_enc_exception("unreachable code!!");
  }
  return (z3::expr)(tmp_v);
}


void symbolic_event::set_pre_events( se_set& prev_events_) {
  prev_events = prev_events_;
}

void symbolic_event::add_post_events( se_ptr& e, z3::expr cond ) {
  post_events.insert( hb_enc::depends( e, cond) );
}


void symbolic_event::set_data_dependency( const hb_enc::depends_set& deps ) {
  data_dependency.clear();
  data_dependency.insert( deps.begin(), deps.end() );
}

void symbolic_event::set_ctrl_dependency( const hb_enc::depends_set& deps ) {
  ctrl_dependency.clear();
  ctrl_dependency.insert( deps.begin(), deps.end() );
}

void symbolic_event::debug_print( std::ostream& stream ) {
  stream << *this << "\n";
  if( et == event_t::r || et == event_t::w ) {
    std::string s = et == event_t::r ? "Read" : "Write";
    stream << s << " var: " << v << ", orig_var : " <<prog_v
           << ", loc :" << loc_name << "\n";
    stream << "guard: " << guard << "\n";
  }
}

//----------------------
// todo: enable memoization
//----------------------
bool tara::hb_enc::is_po_new( const se_ptr& x, const se_ptr& y ) {
  if( x == y ) return true;
  if( x->is_pre() || y->is_post() ) return true;
  if( x->is_post() || y->is_pre() ) return false;
  if( x->tid != y->tid ) return false;
  if( x->get_topological_order() >= y->get_topological_order() ) return false;
  se_set visited, pending = y->prev_events;
  while( !pending.empty() ) {
    se_ptr yp = *pending.begin();
    pending.erase( yp );
    visited.insert( yp );
    if( x == yp ) return true;
    if( x->get_topological_order() >= yp->get_topological_order() ) continue;
    for( auto& ypp : yp->prev_events ) {
      if( visited.find( ypp ) == visited.end() ) pending.insert( ypp );
    }
  }
  return false;
}


//------------------------------------------------------------------------
// depends set utilities

void
hb_enc::pointwise_and( const hb_enc::depends_set& dep_in,
                       z3::expr cond,
                       hb_enc::depends_set& dep_out ) {
  dep_out.clear();
  for( const hb_enc::depends& d : dep_in ) {
    z3::expr c = d.cond && cond;
    c = c.simplify();
    dep_out.insert( hb_enc::depends( d.e, c ));
  }
}

void
hb_enc::join_depends_set( const std::vector<hb_enc::depends_set>& dep,
                          const std::vector<z3::expr>& conds,
                          hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  if( num >= 1 ) pointwise_and( dep.at(0), conds.at(0), result );
  for( unsigned i = 1 ; i < num ; i++ ) {
    hb_enc::depends_set tmp;
    pointwise_and( dep.at(i), conds.at(i), tmp );
    join_depends_set( tmp, result );
  }
}

void
hb_enc::join_depends_set( const std::vector<hb_enc::depends_set>& dep,
                          hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  // hb_enc::depends_set data_dep_ses;
  if ( num == 1 )
    result = dep.at(0);
  else if ( num == 2 )
    join_depends_set( dep.at(0), dep.at( 1 ), result );
  else if ( num > 2 ) {
    // data_dep_ses = join_depends_set( dep.at(0), dep.at( 1 ), result );
    for ( unsigned i = 0 ; i < num ; i++ ) {
      join_depends_set( dep.at( i ), result );
    }
  }
  // return data_dep_ses;
}

//todo : make depends set efficient
//       make it a well implemented class

void hb_enc::join_depends_set( const hb_enc::se_ptr& e1, const z3::expr cond1,
                               depends_set& set ) {
  z3::expr cond = cond1;
  for( auto it1 = set.begin(); it1 != set.end(); it1++) {
    const hb_enc::depends& dep2 = *it1;
    hb_enc::se_ptr e2 = dep2.e;
    if( e1 == e2 ) {
      cond = ( cond || dep2.cond );
      cond = cond.simplify();
      set.erase(it1);
      break;
    }
  }
  set.insert( hb_enc::depends( e1, cond ) );

}

void hb_enc::join_depends_set( const hb_enc::depends& dep,
                               hb_enc::depends_set& set ) {
  join_depends_set( dep.e, dep.cond, set );
}

void hb_enc::join_depends_set( const hb_enc::depends_set& dep0,
                               hb_enc::depends_set& dep1 ) {
  for( auto element0 : dep0 )
    hb_enc::join_depends_set( element0, dep1 );
  // tara::debug_print( std::cout, dep1 );
}


void hb_enc::join_depends_set( const hb_enc::depends_set& dep0,
                               const hb_enc::depends_set& dep1,
                               hb_enc::depends_set& set ) {
  set = dep1;
  join_depends_set( dep0, set );
}

void hb_enc::meet_depends_set( const hb_enc::se_ptr& e1, const z3::expr cond1,
                               depends_set& set ) {
  z3::expr cond = cond1;
  for( auto it1 = set.begin(); it1 != set.end(); it1++) {
    const hb_enc::depends& dep2 = *it1;
    hb_enc::se_ptr e2 = dep2.e;
    if( e1 == e2 ) {
      cond = ( cond && dep2.cond );
      cond = cond.simplify();
      set.erase(it1);
      break;
    }
  }
  set.insert( hb_enc::depends( e1, cond ) );
}

void hb_enc::meet_depends_set( const hb_enc::depends& dep,
                               hb_enc::depends_set& set ) {
  meet_depends_set( dep.e, dep.cond, set );
}

void hb_enc::meet_depends_set( const hb_enc::depends_set& dep0,
                               hb_enc::depends_set& dep1 ) {
  for( auto element0 : dep0 )
    hb_enc::meet_depends_set( element0, dep1 );
}

void hb_enc::meet_depends_set( const hb_enc::depends_set& dep0,
                               const hb_enc::depends_set& dep1,
                               hb_enc::depends_set& set ) {
  set = dep1;
  meet_depends_set( dep0, set );
}


void hb_enc::meet_depends_set( const std::vector<hb_enc::depends_set>& dep,
                               hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  if( num >= 1 ) result =  dep.at(0);
  for( unsigned i = 1 ; i < num ; i++ )
    meet_depends_set( dep.at(i), result );
  // return data_dep_ses;
}

//-------------------------------------------------------------------------

hb_enc::depends
hb_enc::pick_maximal_depends_set( hb_enc::depends_set& set ) {
  //todo : get depends set ordered
  unsigned ord = 0;
  depends d = *set.begin(); //todo : dummy initialization
  for( auto& dep : set ) {
    if( dep.e->get_topological_order() > ord ) d = dep;
  }
  set.erase( d );
  return d;
}

//------------------------------------------------------------------------

void tara::debug_print( std::ostream& out, const hb_enc::se_set& set ) {
  for (auto c : set) {
    out << *c << " ";
  }
  out << std::endl;
}

void tara::debug_print( std::ostream& out, const hb_enc::depends& dep ) {
  out << "("<< *(dep.e) << ",";
  debug_print( out, dep.cond );
  out << ")";
}

void tara::debug_print( std::ostream& out, const hb_enc::depends_set& set ) {
  for (auto dep : set) {
    debug_print( out, dep );
  }
  out << std::endl;
}

