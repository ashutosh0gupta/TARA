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

integer::integer(helpers::z3interf& z3) : encoding(z3)
{}

// integer::integer(z3::context& ctx) : encoding(ctx)
// {}

void integer::record_event( se_ptr& e ) {
  make_tstamp( e->e_v );
  make_tstamp( e->thin_v );
  make_po_tstamp( e->c11_hb_v );
  event_lookup.insert( make_pair( e->get_solver_symbol(), e ) );
  event_lookup.insert( make_pair( e->get_thin_solver_symbol(), e ) );
  event_lookup.insert( make_pair( e->get_c11_hb_solver_symbol(), e ) );
}


void integer::make_tstamp( tstamp_var_ptr loc )
{
  std::vector< tstamp_var_ptr > tstamps;
  tstamps.push_back( loc );
  add_time_stamps(tstamps);
}

void integer::add_time_stamps(vector< tstamp_var_ptr > tstamps)
{
  for (unsigned i=0; i<tstamps.size(); i++) {
    counter++;
    tstamps[i]->_serial = counter;
    z3::expr loc_expr = z3.c.int_const(tstamps[i]->name.c_str());
    tstamps[i]->expr = loc_expr;
    tstamp_lookup.insert(make_pair(loc_expr, tstamps[i]));
  }
  save_tstamps(tstamps);
}

void integer::make_po_tstamp( tstamp_var_ptr loc )
{
  counter++;
  loc->_serial = counter;
  z3::expr loc_expr = z3.c.fresh_constant( loc->name.c_str(), hb_sort );
  loc->expr = loc_expr;
  tstamp_lookup.insert(make_pair(loc_expr, loc));
}


hb integer::make_hb(hb_enc::tstamp_ptr loc1, hb_enc::tstamp_ptr loc2) const
{
  return hb(loc1, loc2, loc1->expr < loc2->expr);
}

hb integer::make_hb_po(hb_enc::tstamp_ptr loc1, hb_enc::tstamp_ptr loc2) const {
  z3::expr e = sr_po_ao( loc1->expr,loc2->expr );
  return hb(loc1, loc2, sr_po(loc1->expr,loc2->expr) );
}

as integer::make_as(hb_enc::tstamp_ptr loc1, hb_enc::tstamp_ptr loc2) const
{
  assert (loc1->thread == loc2->thread);
  return as(loc1, loc2, loc1->expr + (loc2->instr_no-loc1->instr_no)  == loc2->expr);
}

bool integer::eval_hb(const z3::model& model, hb_enc::tstamp_ptr loc1, hb_enc::tstamp_ptr loc2) const
{
  return model.eval(make_hb(loc1, loc2)).get_bool();
}

pair<integer::mapit,integer::mapit>
integer::get_locs( const z3::expr& hb,
                   bool& possibly_equal,
                   bool& is_partial ) const {
  // we need to flip that bool parameter in to know if we are sure about
  // this result or not (two tstamps can be assigned an equal integer)
  auto loc1 = tstamp_lookup.end();
  auto loc2 = tstamp_lookup.end();
  switch(hb.kind()) {
  case Z3_APP_AST: {
    z3::func_decl d = hb.decl();
    Z3_decl_kind dk = d.decl_kind();
    switch(dk) {
    case Z3_OP_LE:
      possibly_equal = true; // fallthrough
    case Z3_OP_LT:
      if (hb.arg(1).kind() == Z3_NUMERAL_AST)
        return get_locs(hb.arg(0), possibly_equal, is_partial);
      loc1 = tstamp_lookup.find(hb.arg(0));
      loc2 = tstamp_lookup.find(hb.arg(1));
      break;
    case Z3_OP_GE:
      possibly_equal = true; // fallthrough
    case Z3_OP_GT:
      if (hb.arg(1).kind() == Z3_NUMERAL_AST) {
        return swap_pair(get_locs(hb.arg(0), possibly_equal, is_partial));
      }
      loc1 = tstamp_lookup.find(hb.arg(1));
      loc2 = tstamp_lookup.find(hb.arg(0));
      break;
    case Z3_OP_NOT: {
      auto neg_hb = get_locs(hb.arg(0), possibly_equal, is_partial);
      auto res = swap_pair( neg_hb );
      possibly_equal = !possibly_equal;
      return res;
      break;
    }
    case Z3_OP_ADD: {
      loc1 = tstamp_lookup.find(hb.arg(0));
      z3::expr t1 = hb.arg(1);
      if (t1.decl().decl_kind() == Z3_OP_MUL) {
        // todo: check the the other multiplicant is -1
        loc2 = tstamp_lookup.find(t1.arg(1));
      }
      break;
    }
    case Z3_OP_SPECIAL_RELATION_PO: {
      is_partial = true;
      loc1 = tstamp_lookup.find(hb.arg(0));
      loc2 = tstamp_lookup.find(hb.arg(1));
      break;
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

hb_ptr integer::get_hb(const z3::expr& hb, bool allow_equal) const
{
  bool possibly_equal = false;
  bool is_partial = false;
  z3::expr hb_p = hb;
  if( z3.is_implies( hb ) ) {
    hb_p = hb.arg(1);
  }
  if( z3.is_bool_const( hb_p ) ) {
    auto it = current_rf_map.find( z3.get_top_func_name( hb_p ) );
    if( it != current_rf_map.end() ) {
      return it->second;
    }
  }

  auto p = get_locs(hb, possibly_equal, is_partial);
  integer::mapit loc1 = p.first, loc2 = p.second, end = tstamp_lookup.end();
  if( (!possibly_equal || allow_equal || is_partial) && loc1!=end && loc2!=end){
    hb_enc::tstamp_ptr l1 = get<1>(*loc1);
    hb_enc::tstamp_ptr l2 = get<1>(*loc2);
    se_ptr e1;
    if( event_lookup.find( l1->expr ) != event_lookup.end() )
      e1 = event_lookup.at( l1->expr );
    se_ptr e2;
    if( event_lookup.find( l2->expr ) != event_lookup.end() )
      e2 = event_lookup.at( l2->expr );
    if( is_partial ){
      return shared_ptr<hb_enc::hb>(new hb_enc::hb( e1, l1, e2, l2, hb,
                                                    possibly_equal,is_partial));
    }else{
      return shared_ptr<hb_enc::hb>(new hb_enc::hb(e1, l1, e2, l2, hb,
                                                   possibly_equal));
    }
  } else
    return shared_ptr<hb_enc::hb>();
}

// unused function set for deprecation
// interesting code - it has annonymous function
vector< tstamp_ptr > integer::get_trace(const z3::model& m) const
{
  vector<tstamp_ptr> result;
  for (auto l : tstamp_map)
    result.push_back(get<1>(l));
  sort(result.begin(), result.end(), [&](const tstamp_ptr & a, const tstamp_ptr & b) -> bool
  { 
    return m.eval(((z3::expr)(*a)) > *b).get_bool(); 
  });
  return result;
}

