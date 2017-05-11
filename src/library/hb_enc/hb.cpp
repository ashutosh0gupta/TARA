/*
 * Copyright 2016, TIFR
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

#include "encoding.h"
#include "symbolic_event.h"
#include "hb.h"
#include <boost/iterator/iterator_concepts.hpp>


using namespace std;

namespace tara {
namespace hb_enc {

/*************************
 * hb
 *************************/

hb::operator z3::expr () const {
  return expr;
}

  z3::expr hb::get_guarded_forbid_expr() {
    return implies( e1->guard && e2->guard, !expr );
  }

  void hb::update_signature() {
    _signature = loc1->serial();
    _signature <<= 16;
    _signature |= loc2->serial();
  }

hb::hb(tstamp_ptr loc1_, tstamp_ptr loc2_, z3::expr expr):
  loc1(loc1_), loc2(loc2_), expr(expr)
{
  update_signature();
  // _signature = loc1->serial();
  // _signature <<= 16;
  // _signature |= loc2->serial();
}

  //the following allocator is not in use
hb::hb(se_ptr e1_, se_ptr e2_, z3::expr expr):
  e1( e1_), e2( e2_), loc1(e1->e_v), loc2(e2->e_v),
  is_neg( false ), is_partial( false ), type( hb_t::hb ),
  expr(expr)
{
  update_signature();
}

hb::hb( se_ptr e1_, tstamp_ptr l1_,
        se_ptr e2_, tstamp_ptr l2_, z3::expr expr,
        bool is_neg ):
  e1( e1_), e2( e2_), loc1(l1_), loc2(l2_),
  is_neg( is_neg ), is_partial( false ), type( hb_t::hb ),
  expr(expr)
{
 if( loc1->name.find( "__thin__" ) == 0 ) { type = hb_t::thin;
  }
  update_signature();
}

  // todo: this call should be removed
hb::hb( se_ptr e1_, tstamp_ptr l1_,
        se_ptr e2_, tstamp_ptr l2_, z3::expr expr,
        bool is_neg, bool is_partial ):
  e1( e1_), e2( e2_), loc1(l1_), loc2(l2_),
  is_neg(is_neg) , is_partial( is_partial ), type( hb_t::hb ),
  expr(expr)
{
  if( is_partial ) { type = hb_t::phb;
  }else if( loc1->name.find( "__thin__" ) == 0 ) { type = hb_t::thin;
  }
  update_signature();
}

hb::hb( se_ptr e1_, se_ptr e2_, z3::expr expr,
        bool is_neg, hb_t type_ ):
  e1( e1_), e2( e2_),loc1( ), loc2( ),
  is_neg(is_neg) , is_partial( false ), type( type_ ),
  expr(expr)
{
  if( type == hb_t::phb ) is_partial = false;
  // update_signature();
}

uint32_t hb::signature()
{
  return _signature;
}

bool hb::operator==(const hb& other) const
{
  return _signature == other._signature;
}

bool hb::operator!=(const hb &other) const {
  return !(*this == other);
}

bool operator< (const hb& hb1, const hb& hb2)
{
  if( hb1.loc1 != nullptr && hb1.loc2 != nullptr &&
      hb2.loc1 != nullptr && hb2.loc2 != nullptr )
    return COMPARE_OBJ2( hb1, hb2, loc1->name, loc2->name );
  return COMPARE_OBJ2( hb1, hb2, e1->name(), e2->name() );
}

ostream& operator<< (std::ostream& stream, const hb& hb) {
  if( hb.type == hb_t::rf ) {
    stream << "rf(" << hb.e1->name() << "," << hb.e2->name() << ")";
  }else if( hb.type == hb_t::thin ) {
    stream << "hb_ar(" << hb.e1->name() << "," << hb.e2->name() << ")";
  }else{
    if( hb.is_partial && hb.is_neg )
      stream << "!hb(" << hb.e2->name() << "," << hb.e1->name() << ")";
    else if( hb.is_partial && !hb.is_neg )
      stream << "hb(" << hb.e1->name() << "," << hb.e2->name() << ")";
    else
      // stream << "hb(" << hb.loc1 << "," << hb.loc2 << ")";
      stream << "(" << hb.loc1 << " < " << hb.loc2 << ")";
  }
  return stream;
}

void hb::debug_print(std::ostream& stream ) {  stream << *this << "\n"; }

hb hb::negate() const
{
  return hb(loc2, loc1, !expr);
}


}
  void debug_print( std::ostream& out, const hb_enc::hb_vec& hbs) {
    for( auto& hb : hbs ) {
      out << *hb << " ";
    }
  }

}
