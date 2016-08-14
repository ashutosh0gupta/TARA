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
#include "symbolic_event.h"
#include <boost/iterator/iterator_concepts.hpp>

using namespace std;

namespace tara {
namespace hb_enc {

/*************************
 * location
 *************************/
  
location::location(z3::context& ctx, string name, bool special) : expr(z3::expr(ctx)), name(name) , special(special) {}
//location::location(z3::context& ctx, string name ,bool special) : expr(z3::expr(ctx)), name(name),special(special) {}

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

hb::hb(se_ptr e1_, se_ptr e2_, z3::expr expr):
  e1( e1_), e2( e2_), loc1(e1->e_v), loc2(e2->e_v), expr(expr)
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

bool operator< (const hb& hb1, const hb& hb2)
{
  return hb1.loc1->name < hb2.loc1->name ||
    ( hb1.loc1->name ==  hb2.loc1->name &&
      hb1.loc2->name < hb2.loc2->name );
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

encoding::encoding(z3::context& _ctx) :ctx(_ctx)
{}

encoding::~encoding()
{}


  //location_map is only used in old code
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

hb encoding::mk_hbs(const se_ptr& before, const se_ptr& after) {
  return make_hb( before->e_v, after->e_v );
}


// ghb = guarded hb
z3::expr encoding::mk_ghbs( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hbs( before, after ) );
}

z3::expr encoding::mk_ghbs( const se_ptr& before, const se_set& after ) {
  z3::expr hbs = ctx.bool_val(true);
  for( auto& af : after ) hbs = hbs && mk_ghbs( before, af );
  return hbs;
}

z3::expr encoding::mk_ghbs( const se_set& before, const se_ptr& after ) {
  z3::expr hbs = ctx.bool_val(true);
  for( auto& bf : before ) hbs = hbs && mk_ghbs( bf, after );
  return hbs;
}

// thin air ghb
z3::expr encoding::mk_ghb_thin( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hb_thin( before, after ) );
}

z3::expr encoding::mk_hbs( const se_set& before, const se_ptr& after) {
  z3::expr hbs = ctx.bool_val(true);
  for( auto& bf : before ) {
      hbs = hbs && mk_hbs( bf, after );
  }
  return hbs;
}

z3::expr encoding::mk_hbs(const se_ptr& before, const se_set& after) {
  z3::expr hbs = ctx.bool_val(true);
  for( auto& af : after ) {
      hbs = hbs && mk_hbs( before, af );
  }
  return hbs;
}


z3::expr encoding::mk_hbs(const se_set& before, const se_set& after) {
  z3::expr hbs = ctx.bool_val(true);
  for( auto& bf : before ) {
    for( auto& af : after ) {
      hbs = hbs && mk_hbs( bf, af );
    }
  }
  return hbs;
}

hb encoding::mk_hb_thin(const se_ptr& before, const se_ptr& after) {
  return make_hb( before->thin_v, after->thin_v );
}

bool encoding::eval_hb( const z3::model& m,
                        const se_ptr& before, const se_ptr& after ) const{
  return eval_hb( m, before->e_v, after->e_v );
}

list<z3::expr> encoding::get_hbs( z3::model& m ) const
{
  list<z3::expr> result;
  z3::expr_vector asserted = ctx.collect_last_asserted_linear_constr();
  
  for( unsigned i = 0; i<asserted.size(); i++ ) {
    z3::expr atom = asserted[i];
    unique_ptr<hb_enc::hb> hb = get_hb( atom );
    if (hb && !hb->loc1->special && !hb->loc2->special && hb->loc1->thread != hb->loc2->thread) {
      //z3::expr hb2 = _hb_encoding.make_hb(hb->loc1, hb->loc2);
      //cout << asserted[i] << " | " << (z3::expr)*hb << " | " << *hb << " | " << (z3::expr)hb2 << endl;
      //assert(m.eval(*hb).get_bool() == m.eval(hb2).get_bool());
      assert(eval_hb(m, hb->loc1, hb->loc2));
      result.push_back(asserted[i]);
    }
  }
  
  return result;
}


}}
