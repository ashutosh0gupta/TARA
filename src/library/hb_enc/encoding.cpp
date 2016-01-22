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

#include "encoding.h"
#include <boost/iterator/iterator_concepts.hpp>

using namespace std;

namespace tara {
namespace hb_enc {

/*************************
 * location
 *************************/
  
location::location(z3::context& ctx, string name, bool special) : expr(z3::expr(ctx)), name(name), special(special) {}

bool location::operator==(const location &other) const {
  // if expr is empty fall back to name
  if ((Z3_ast)this->expr==0) 
    return this->name == other.name;
  else
    return (Z3_ast)this->expr == (Z3_ast)other.expr;
}

bool location::operator!=(const location &other) const {
  return !(*this == other);
}

location::operator z3::expr () const {
  return expr;
}

uint16_t location::serial() const
{
  return _serial;
}

string to_string (const location& loc) {
  string res = loc.name;
  return res;
}


string get_var_from_location (location_ptr loc) {
  string res = loc->name;
unsigned i;
  for(i=2;i!=res.length();i++)
  {

	  if(res[i]=='#')
	  {
		  res=res.substr(2,i-2);
		  break;
	  }
  }

  return res;
}


ostream& operator<< (ostream& stream, const location& loc) {
  stream << to_string(loc);
  return stream;
}

ostream& operator<< (ostream& stream, const location_ptr& loc) {
  stream << *loc;
  return stream;
}

string operator+ (const string& lhs, const location_ptr& rhs) {
  return lhs + to_string(*rhs);
}

ostream& operator<< (ostream& stream, const location_pair& loc_pair) {
  stream << loc_pair.first << "-" << loc_pair.second;
  return stream;
}

void location::debug_print(std::ostream& stream ) {  stream << *this << "\n"; }

/*************************
 * hb
 *************************/

hb::operator z3::expr () const {
  return expr;
}

hb::hb(location_ptr location1, location_ptr location2, z3::expr expr):
  loc1(location1), loc2(location2), expr(expr)
{
  _signature = loc1->serial();
  _signature <<= 16;
  _signature |= loc2->serial();
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

ostream& operator<< (std::ostream& stream, const hb& hb) {
  stream << "hb(" << hb.loc1 << "," << hb.loc2 << ")";
  return stream;
}

  void hb::debug_print(std::ostream& stream ) {  stream << *this << "\n"; }

hb hb::negate() const
{
  return hb(loc2, loc1, !expr);
}


/*************************
 * as
 *************************/


as::operator z3::expr () const {
  return expr;
}

as::as(location_ptr location1, location_ptr location2, z3::expr expr):
loc1(location1), loc2(location2), expr(expr)
{
  _signature = loc1->serial();
  _signature <<= 16;
  _signature |= loc2->serial();
}

uint32_t as::signature()
{
  return _signature;
}

bool as::operator==(const as& other) const
{
  return _signature == other._signature;
}

bool as::operator!=(const as &other) const {
  return !(*this == other);
}

ostream& operator<< (std::ostream& stream, const as& as) {
  stream << "as(" << as.loc1 << "," << as.loc2 << ")";
  return stream;
}

/*************************
 * encoding
 *************************/

encoding::encoding()
{}

encoding::~encoding()
{}


location_ptr encoding::get_location(const string& name) const
{
  auto found = location_map.find(name);
  assert (found != location_map.end());
  return get<1>(*found);
}

void encoding::save_locations(const vector< shared_ptr< location > >& locations)
{
  location_map.clear();
  for (auto loc: locations) {
    location_map.insert(make_pair(loc->name, loc));
  }
}

}}
