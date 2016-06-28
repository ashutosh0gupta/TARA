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
#include "hb_enc/hb_enc_exception.h"

using namespace tara;
using namespace tara::hb_enc;
using namespace tara::helpers;

using namespace std;

//----------------------------------------------------------------------------
// utilities for symbolic events


std::shared_ptr<tara::hb_enc::location>
symbolic_event::create_internal_event( z3::context& ctx,
                                       hb_enc::encoding& _hb_enc,
                                       std::string& event_name,
                                       unsigned tid,
                                       unsigned instr_no,
                                       bool special,
                                       bool is_read,
                                       std::string& prog_v_name )
{
  auto e_v = make_shared<hb_enc::location>( ctx, event_name, special );
  e_v->thread = tid;
  e_v->instr_no = instr_no;
  e_v->is_read = is_read;
  e_v->prog_v_name = prog_v_name;
  std::vector< std::shared_ptr<tara::hb_enc::location> > locations;
  locations.push_back( e_v );
  _hb_enc.make_locations(locations);
  return e_v;
}

symbolic_event::symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                                unsigned _tid, unsigned instr_no,
                                const cssa::variable& _v, const cssa::variable& _prog_v,
                                hb_enc::location_ptr _loc, event_kind_t _et )
  : tid(_tid)
  , v(_v)
  , prog_v( _prog_v )
  , loc(_loc)
  , et( _et )
  , guard(ctx)
{
  if( et != event_kind_t::r &&  event_kind_t::w != et ) {
    throw hb_enc_exception("symboic event with wrong parameters!");
  }
  // bool special = (event_kind_t::i == et || event_kind_t::f == et);
  bool is_read = (et == event_kind_t::r); // || event_kind_t::f == et);
  std::string et_name = is_read ? "R" : "W";
  std::string event_name = et_name + "#" + v.name;
  e_v = create_internal_event( ctx, _hb_enc, event_name, _tid,instr_no, false,
                               is_read, prog_v.name );
  event_name = "__thin__" + event_name;
  thin_v = create_internal_event( ctx, _hb_enc, event_name, _tid, instr_no, false,
                                  is_read, prog_v.name );
}


// barrier events
symbolic_event::symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                                unsigned _tid, unsigned instr_no,
                                hb_enc::location_ptr _loc, event_kind_t _et )
  : tid(_tid)
  , v("dummy",ctx)
  , prog_v( "dummy",ctx)
  , loc(_loc)
  , et( _et )
  , guard(ctx)
{
  std::string event_name;
  switch( et ) {
  case event_kind_t::barr: { event_name = "#barr";  break; }
  case event_kind_t::pre : { event_name = "#pre" ;  break; }
  case event_kind_t::post: { event_name = "#post";  break; }
  default: hb_enc_exception("unreachable code!!");
  }
  event_name = loc->name+event_name;
  e_v = create_internal_event( ctx, _hb_enc, event_name, _tid, instr_no, true,
                               (event_kind_t::post == et), prog_v.name );
  std::string thin_name = "__thin__" + event_name;
  thin_v = create_internal_event( ctx, _hb_enc, thin_name, tid, instr_no, true,
                                  (event_kind_t::post == et), prog_v.name );
}

z3::expr symbolic_event::get_var_expr( const cssa::variable& g ) {
  assert( et != event_kind_t::barr);
  if( et == event_kind_t::r || et == event_kind_t::w) {
    z3::expr v_expr = (z3::expr)(v);
    return v_expr;
  }
  cssa::variable tmp_v(g.sort.ctx());
  switch( et ) {
  // case event_kind_t::barr: { tmp_v = g+"#barr";  break; }
  case event_kind_t::pre : { tmp_v = g+"#pre" ;  break; }
  case event_kind_t::post: { tmp_v = g+"#post";  break; }
  default: hb_enc_exception("unreachable code!!");
  }
  return (z3::expr)(tmp_v);
}

void symbolic_event::debug_print(std::ostream& stream ) {
  stream << *this << "\n";
  if( et == event_kind_t::r || et == event_kind_t::w ) {
    std::string s = et == event_kind_t::r ? "Read" : "Write";
    stream << s << " var: " << v << ", orig_var : " <<prog_v
           << ", loc :" << loc << "\n";
    stream << "guard: " << guard << "\n";
  }
}



void tara::hb_enc::debug_print_se_set( const hb_enc::se_set& set,
                                 std::ostream& out ) {
  for (auto c : set) {
    out << *c << " ";
    }
  out << std::endl;
}

