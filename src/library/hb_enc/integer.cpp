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

#include "helpers/helpers.h"
#include "integer.h"
#include <algorithm>

using namespace tara::hb_enc;
using namespace tara::helpers;
using namespace tara;

using namespace std;

integer::integer(z3::context& ctx) : encoding(ctx) //, ctx(ctx)
{}

void integer::record_event( se_ptr& e ) {
  make_location( e->e_v );
  make_location( e->thin_v );
  // event_lookup.insert( make_pair( e->get_solver_symbol(), e ) );
  // event_lookup.insert( make_pair( e->get_thin_solver_symbol(), e ) );
}

void integer::make_location( std::shared_ptr< location > loc )
{
  std::vector< std::shared_ptr<tara::hb_enc::location> > locations;
  locations.push_back( loc );
  make_locations(locations);
}

void integer::make_locations(vector< std::shared_ptr< location > > locations)
{
  // todo: should the following be static??
  uint16_t counter = 0;
  for (unsigned i=0; i<locations.size(); i++) {
    counter++;
    locations[i]->_serial = counter;
    z3::expr loc_expr = ctx.int_const(locations[i]->name.c_str());
    locations[i]->expr = loc_expr;
    location_lookup.insert(make_pair(loc_expr, locations[i]));
  }
  save_locations(locations);
}


hb integer::make_hb(std::shared_ptr< const hb_enc::location > loc1, std::shared_ptr< const hb_enc::location > loc2) const
{
  return hb(loc1, loc2, loc1->expr < loc2->expr);
}

as integer::make_as(std::shared_ptr< const hb_enc::location > loc1, std::shared_ptr< const hb_enc::location > loc2) const
{
  assert (loc1->thread == loc2->thread);
  return as(loc1, loc2, loc1->expr + (loc2->instr_no-loc1->instr_no)  == loc2->expr);
}

bool integer::eval_hb(const z3::model& model, std::shared_ptr< const hb_enc::location > loc1, std::shared_ptr< const hb_enc::location > loc2) const
{
  return model.eval(make_hb(loc1, loc2)).get_bool();
}

pair<integer::mapit,integer::mapit> integer::get_locs(const z3::expr& hb, bool& possibly_equal) const {
  // we need to flip that bool parameter in to know if we are sure about this result or not (two locations can be assigned an equal integer)
  auto loc1 = location_lookup.end();
  auto loc2 = location_lookup.end();
  switch(hb.kind()) {
    case Z3_APP_AST:
    {
      z3::func_decl d = hb.decl();
      Z3_decl_kind dk = d.decl_kind();
      switch(dk) {
        case Z3_OP_LE:
          possibly_equal = true; // fallthrough
        case Z3_OP_LT: 
          if (hb.arg(1).kind() == Z3_NUMERAL_AST)
            return get_locs(hb.arg(0), possibly_equal);
          loc1 = location_lookup.find(hb.arg(0));
          loc2 = location_lookup.find(hb.arg(1));
          break;
        case Z3_OP_GE: 
          possibly_equal = true; // fallthrough
        case Z3_OP_GT: 
          if (hb.arg(1).kind() == Z3_NUMERAL_AST) {
            return swap_pair(get_locs(hb.arg(0), possibly_equal));
          }
          loc1 = location_lookup.find(hb.arg(1));
          loc2 = location_lookup.find(hb.arg(0));
          break;
        case Z3_OP_NOT:
        {
          auto res = swap_pair(get_locs(hb.arg(0), possibly_equal));
          possibly_equal = !possibly_equal;
          return res;
          break;
        }
        case Z3_OP_ADD:
        {
          loc1 = location_lookup.find(hb.arg(0));
          z3::expr t1 = hb.arg(1);
          if (t1.decl().decl_kind() == Z3_OP_MUL) {
            loc2 = location_lookup.find(t1.arg(1));
          }
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return make_pair<integer::mapit,integer::mapit>(std::move(loc1),std::move(loc2));
}

unique_ptr<hb> integer::get_hb(const z3::expr& hb, bool allow_equal) const
{
  bool possibly_equal = false;
  auto p = get_locs(hb, possibly_equal);
  integer::mapit loc1 = p.first;
  integer::mapit loc2 = p.second;
  if ((!possibly_equal || allow_equal) && loc1 != location_lookup.end() && loc2 != location_lookup.end()) {
    std::shared_ptr<hb_enc::location> l1 = get<1>(*loc1);
    std::shared_ptr<hb_enc::location> l2 = get<1>(*loc2);
    // se_ptr e1;
    // if( event_lookup.find( l1->expr ) != event_lookup.end() )
    //   e1 = event_lookup.at( l1->expr );
    // se_ptr e2;
    // if( event_lookup.find( l2->expr ) != event_lookup.end() )
    //   e2 = event_lookup.at( l2->expr );
    return unique_ptr<hb_enc::hb>(new hb_enc::hb(l1, l2, hb));
  } else 
    return unique_ptr<hb_enc::hb>();
}

// unused function set for deprecation
// interesting code - it has annonymous function
vector< location_ptr > integer::get_trace(const z3::model& m) const
{
  vector<location_ptr> result;
  for (auto l : location_map)
    result.push_back(get<1>(l));
  sort(result.begin(), result.end(), [&](const location_ptr & a, const location_ptr & b) -> bool
  { 
    return m.eval(((z3::expr)(*a)) > *b).get_bool(); 
  });
  return result;
}

