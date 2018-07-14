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
#include "hb_enc/hb.h"

using namespace tara;
using namespace tara::hb_enc;
using namespace tara::helpers;

using namespace std;


/*************************
 * tstamp
 *************************/
  
tstamp::tstamp(z3::context& ctx, string name, bool special) : expr(z3::expr(ctx)), name(name) , special(special) {}

bool tstamp::operator==(const tstamp &other) const {
  // if expr is empty fall back to name
  if ((Z3_ast)this->expr==0) 
    return this->name == other.name;
  else
    return (Z3_ast)this->expr == (Z3_ast)other.expr;
}

bool tstamp::operator!=(const tstamp &other) const {
  return !(*this == other);
}

tstamp::operator z3::expr () const {
  return expr;
}

uint16_t tstamp::serial() const
{
  return _serial;
}

string tara::hb_enc::to_string(const tstamp& loc) {
  string res = loc.name;
  return res;
}


ostream& tara::hb_enc::operator<< (ostream& stream, const tstamp& loc) {
  stream << to_string(loc);
  return stream;
}

ostream& tara::hb_enc::operator<< (ostream& stream, const tstamp_ptr& loc) {
  stream << *loc;
  return stream;
}

string operator+ (const string& lhs, const tstamp_ptr& rhs) {
  return lhs + to_string(*rhs);
}

ostream& tara::hb_enc::operator<< ( ostream& stream,
                                    const tstamp_pair& loc_pair)
{
  stream << loc_pair.first << "-" << loc_pair.second;
  return stream;
}

void tstamp::debug_print(std::ostream& stream ) {  stream << *this << "\n"; }


//----------------------------------------------------------------------------
// utilities for symbolic events

std::string hb_enc::event_t_name( event_t et ) {
    switch( et) {
    case event_t::pre   : return "pre"  ; break;
    case event_t::r     : return "R"    ; break;
    case event_t::w     : return "W"    ; break;
    case event_t::u     : return "U"    ; break;
    case event_t::barr  : return "barr" ; break;
    case event_t::barr_b: return "barr_b" ; break;
    case event_t::barr_a: return "barr_a" ; break;
    case event_t::block : return "block"; break;
    case event_t::post  : return "post" ; break;
    case event_t::create: return "create"; break;
    case event_t::join  : return "join" ; break;
    default: hb_enc_error( "name of the event type is not implemented yet!" );
    }
}

std::string source_loc::position_name() {
  if( line == 0 && col == 0 )
    return pretty_name;
  else
    return "_l" + std::to_string(line) + "_c" + std::to_string(col);
}

std::string source_loc::gen_name() {
  static std::map< std::pair<unsigned, unsigned>, unsigned > seen_before;
  auto l_name = position_name();
  if( line == 0 && col == 0 && l_name != "" ) {
    return l_name;
  }else{
    auto line_col = std::make_pair(line, col);
    if( exists( seen_before, line_col ) ) {
      return l_name + "_u" + std::to_string( seen_before[line_col]++ );
    }else{
      seen_before[line_col] = 0;
    }
    return l_name;
  }
}

tstamp_var_ptr
symbolic_event::create_internal_event( helpers::z3interf& z3,
                                       std::string e_name, unsigned tid,
                                       unsigned instr_no, bool special )
{
  auto e_v = make_shared<hb_enc::tstamp>( z3.c, e_name, special );
  e_v->thread = tid;
  e_v->instr_no = instr_no; // not used in post POPL15

  return e_v;
}

std::string symbolic_event::name() const {
  return e_v->name;
}

std::string symbolic_event::get_position_name() const {
  return position_name;
}

unsigned symbolic_event::get_instr_no() const {
  return e_v->instr_no;
}

z3::expr symbolic_event::get_solver_symbol() const {
  return *e_v;
}

z3::expr symbolic_event::get_thin_solver_symbol() const {
  return *thin_v;
}

z3::expr symbolic_event::get_c11_hb_solver_symbol() const {
  return *get_c11_hb_stamp();
}

z3::expr symbolic_event::get_c11_mo_solver_symbol() const {
  return *get_c11_mo_stamp();
}

z3::expr symbolic_event::get_c11_sc_solver_symbol() const {
  return *get_c11_sc_stamp();
}


z3::expr symbolic_event::get_power_hb_solver_symbol() const {
  return *get_power_hb_stamp();
}

z3::expr symbolic_event::get_power_prop_solver_symbol() const {
  return *get_power_prop_stamp();
}

z3::expr symbolic_event::get_power_obs_solver_symbol() const {
  return *get_power_obs_stamp();
}

z3::expr symbolic_event::get_power_mo_solver_symbol() const {
  return *get_power_obs_stamp();
}


void symbolic_event::update_topological_order() {
  unsigned max = 0;
  for( const se_ptr e : prev_events)
    if( max < e->get_topological_order() ) max = e->get_topological_order();
  topological_order = max + 1;
  // o_tag = o_tag_t::na;
}


//Old constructors:
// Move the ctrc calls to the new constructors

symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events, unsigned instr_no,
                                const tara::variable& _v,
                                const tara::variable& _prog_v,
                                std::string _loc, event_t _et )
  : tid(_tid)
  , v(_v)
  , v_copy(_v)
  , prog_v( _prog_v )
  , loc_name(_loc)
  , et( _et )
  , prev_events( _prev_events )
  , guard(z3.c)
{
  if( et != event_t::r &&  et != event_t::w &&  et != event_t::u ) {
    hb_enc_error("symboic event with wrong parameters!");
  }
  if( et == event_t::u ) {
    v_copy = v + "#update_wr";
  }
  std::string e_name = hb_enc::event_t_name( et ) + "#" + v.name;
  e_v = create_internal_event(   z3,            e_name,tid,instr_no,false);
  thin_v = create_internal_event(z3,"__thin__" +e_name,tid,instr_no,false);
  c11_hb_v=create_internal_event(z3,"__hb__"   +e_name,tid,instr_no,false);
  update_topological_order();
  o_tag = o_tag_t::na;
}


// barrier events
symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events, unsigned instr_no,
                                std::string _loc, event_t _et )
  : tid(_tid)
  , v("dummy",z3.c)
  , v_copy("dummy",z3.c)
  , prog_v( "dummy",z3.c)
  , loc_name(_loc)
  , et( _et )
  , prev_events( _prev_events )
  , guard(z3.c)
{
  std::string e_name = hb_enc::event_t_name( et ) + "#" + loc_name;
  e_v = create_internal_event    (z3,            e_name,tid,instr_no,true);
  thin_v = create_internal_event (z3, "__thin__"+e_name,tid,instr_no,true);
  c11_hb_v =create_internal_event(z3, "__hb__"  +e_name,tid,instr_no,true);
  update_topological_order();
  o_tag = o_tag_t::na;
}


// new constructor
symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events,
                                const tara::variable& _prog_v,
                                z3::expr& path_cond,
                                std::vector<z3::expr>& _history,
                                hb_enc::source_loc& _loc, event_t _et,
                                hb_enc::o_tag_t _o_tag )
  : tid(_tid)
  , v(_prog_v)      // temp init
  , v_copy(_prog_v) // temp init
  , prog_v( _prog_v )
  , loc(_loc)
  , et( _et )
  , o_tag( _o_tag )
  , prev_events( _prev_events )
  , guard(path_cond)
  , history( _history )
{

  if( et != event_t::r &&  et != event_t::w &&  et != event_t::u ) {
    hb_enc_error("symboic event with wrong parameters!");
  }

  loc_name = loc.gen_name();
  std::string e_t_name = hb_enc::event_t_name( et );
  position_name = e_t_name + "#" + prog_v.name + "#" + loc.position_name();

  v = prog_v + "#" + loc_name;
  v_copy = (et != event_t::u) ? v : v + "#update_wr";

  std::string e_name = e_t_name + "#" + v.name;
  e_v    = create_internal_event( z3,            e_name,tid, 0, false);
  thin_v = create_internal_event(z3,"__thin__" +e_name,tid, 0, false);
  c11_hb_v=create_internal_event(z3,"__hb__"   +e_name,tid, 0, false);
  update_topological_order();
}

symbolic_event::symbolic_event( helpers::z3interf& z3, unsigned _tid,
                                se_set& _prev_events,
                                z3::expr& path_cond,
                                std::vector<z3::expr>& _history,
                                hb_enc::source_loc& _loc, event_t _et,
                                hb_enc::o_tag_t _o_tag )
  : tid(_tid)
  , v("dummy",z3.c)
  , v_copy("dummy",z3.c)
  , prog_v( "dummy",z3.c)
  , loc(_loc)
  , et( _et )
  , o_tag( _o_tag )
  , prev_events( _prev_events )
  , guard(path_cond)
  , history( _history )
{
  std::string e_t_name = hb_enc::event_t_name( et );

  if( et == hb_enc::event_t::block ) {
    hb_enc::source_loc loc_d;
    loc_name = "block__" + std::to_string(tid) + "__"+ loc_d.gen_name();
  }else{
    loc_name = loc.gen_name();
  }

  position_name = e_t_name + "#" + loc.position_name();

  std::string e_name = e_t_name + "#" + loc_name;
  e_v = create_internal_event    (z3,            e_name,tid,0,true);
  thin_v = create_internal_event (z3, "__thin__"+e_name,tid,0,true);
  c11_hb_v =create_internal_event(z3, "__hb__"  +e_name,tid,0,true);

  update_topological_order();
}

z3::expr symbolic_event::get_rd_expr( const tara::variable& g ) {
  assert( et == event_t::r   || et == event_t::u || et == event_t::post );
  if( et == event_t::r || et == event_t::u ) {
    z3::expr v_expr = (z3::expr)(v);
    return v_expr;
  }
  tara::variable tmp_v = g+"#post";
  return (z3::expr)(tmp_v);
}

z3::expr symbolic_event::get_wr_expr( const tara::variable& g ) {
  assert( et == event_t::u   || et == event_t::w || et == event_t::pre );
  if( et == event_t::w || et == event_t::u ) {
    z3::expr v_expr = (z3::expr)(v_copy);
    return v_expr;
  }
  tara::variable tmp_v(g.sort.ctx());
  switch( et ) {
  // case event_t::barr: { tmp_v = g+"#barr";  break; }
  case event_t::pre : { tmp_v = g+"#pre" ;  break; }
  case event_t::post: { tmp_v = g+"#post";  break; }
  default: hb_enc_error("unreachable code!!");
  }
  return (z3::expr)(tmp_v);
}


void symbolic_event::set_pre_events( se_set& prev_events_) {
  prev_events = prev_events_;
}

void symbolic_event::add_post_events( se_ptr& e, z3::expr cond ) {
  post_events.insert( hb_enc::depends( e, cond) );
}

//todo : make the following function efficient
z3::expr symbolic_event::get_post_cond( const se_ptr& e_post ) const {
  for( const auto& dep : post_events ) {
    if( dep.e == e_post ) return dep.cond;
  }
  assert( false ); // called with wrong parameters
  return guard.ctx().bool_val(false); //dummy code
}


void symbolic_event::set_data_dependency( const hb_enc::depends_set& deps ) {
  data_dependency.clear();
  data_dependency.insert( deps.begin(), deps.end() );
}

void symbolic_event::set_ctrl_dependency( const hb_enc::depends_set& deps ) {
  ctrl_dependency.clear();
  ctrl_dependency.insert( deps.begin(), deps.end() );
}

void symbolic_event::set_addr_dependency( const hb_enc::depends_set& deps ) {
  addr_dependency.clear();
  addr_dependency.insert( deps.begin(), deps.end() );
}

//todo: add addr dependency field
void symbolic_event::set_dependencies( const hb_enc::depends_set& data,
                                       const hb_enc::depends_set& ctrl ) {
  set_data_dependency( data );
  set_ctrl_dependency( ctrl );
}

z3::expr symbolic_event::get_data_dependency_cond( const se_ptr& e2 ) {
  for( auto& dep : data_dependency ) {
    if( dep.e == e2 ) {
       return dep.cond;
      break;
    }
  }
  return z3::expr( guard.ctx() );
}

z3::expr symbolic_event::get_ctrl_dependency_cond( const se_ptr& e2 ) {
  for( auto& dep : ctrl_dependency ) {
    if( dep.e == e2 ) {
       return dep.cond;
      break;
    }
  }
  return z3::expr( guard.ctx() );
}

z3::expr symbolic_event::get_addr_dependency_cond( const se_ptr& e2 ) {
  for( auto& dep : addr_dependency ) {
    if( dep.e == e2 ) {
       return dep.cond;
      break;
    }
  }
  return z3::expr( guard.ctx() );
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
  if( x == y )
    return true;
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

void tara::debug_print( std::ostream& out, const hb_enc::se_vec& set ) {
  for (auto c : set) {
    out << *c << " ";
  }
  out << std::endl;
}

void tara::debug_print( std::ostream& out, const hb_enc::se_set& set ) {
  for (auto c : set) {
    out << *c << " ";
  }
  out << std::endl;
}

void tara::debug_print( std::ostream& out, const hb_enc::se_to_ses_map& map ) {
  for (auto it : map) {
    se_ptr e = it.first;
    se_set set  = it.second;
    out << *e << "->";
    debug_print( out, set );
    out << "\n";
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

void tara::hb_enc::
full_initialize_se( hb_enc::encoding& hb_enc, se_ptr e, se_set& prev_es,
                    std::map<const hb_enc::se_ptr, z3::expr>& branch_conds) {
  hb_enc.record_event( e );
  for(se_ptr ep  : prev_es) {
    ep->add_post_events( e, branch_conds.at(ep) );
  }
}
