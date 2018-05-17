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
#include "hb.h"
#include <boost/iterator/iterator_concepts.hpp>

using namespace std;

namespace tara {
namespace hb_enc {

/*************************
 * as
 *************************/


as::operator z3::expr () const {
  return expr;
}

as::as(tstamp_ptr tstamp1, tstamp_ptr tstamp2, z3::expr expr):
loc1(tstamp1), loc2(tstamp2), expr(expr)
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

encoding::encoding(helpers::z3interf& _z3) :z3(_z3)
{}

// encoding::encoding(z3::context& _ctx) :ctx(_ctx)
// {}

encoding::~encoding()
{}


  //tstamp_map is only used in old code
tstamp_ptr encoding::get_tstamp(const string& name) const
{
  auto found = tstamp_map.find(name);
  assert (found != tstamp_map.end());
  return get<1>(*found);
}

void encoding::save_tstamps(const vector< tstamp_var_ptr >& tstamps)
{
  tstamp_map.clear();
  for (auto loc: tstamps) {
    tstamp_map.insert(make_pair(loc->name, loc));
  }
}

void encoding::record_rf_map(std::set< std::tuple< std::string,hb_enc::se_ptr,
                                                   hb_enc::se_ptr> >& rf_map_)
{
  rf_map = rf_map_;
}

hb encoding::mk_hbs(const se_ptr& before, const se_ptr& after) {
  return make_hb( before->e_v, after->e_v );
}


// ghb = guarded hb
z3::expr encoding::mk_ghbs( const se_ptr& before, const se_ptr& after ) {
  z3::expr cond = before->guard && after->guard;
  cond = cond.simplify();
  return implies( cond, mk_hbs( before, after ) );
}

z3::expr encoding::mk_ghbs( const se_ptr& before, const se_set& after ) {
  z3::expr hbs = z3.mk_true();
  for( auto& af : after ) hbs = hbs && mk_ghbs( before, af );
  return hbs;
}

z3::expr encoding::mk_ghbs( const se_set& before, const se_ptr& after ) {
  z3::expr hbs = z3.mk_true();
  for( auto& bf : before ) hbs = hbs && mk_ghbs( bf, after );
  return hbs;
}

// thin air ghb
z3::expr encoding::mk_ghb_thin( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hb_thin( before, after ) );
}


z3::expr encoding::mk_ghb_c11_hb( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hb_c11_hb( before, after ));
}

z3::expr encoding::mk_ghbs_c11_hb( const se_set& before, const se_ptr& after ) {
  z3::expr hbs = z3.mk_true();
  for( auto& bf : before ) hbs = hbs && mk_ghb_c11_hb( bf, after );
  return hbs;
}

z3::expr encoding::mk_ghb_c11_mo( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hb_c11_mo( before, after ));
}

z3::expr encoding::mk_ghb_c11_sc( const se_ptr& before, const se_ptr& after ) {
  return implies( before->guard && after->guard, mk_hb_c11_sc( before, after ));
}

z3::expr encoding::mk_hbs( const se_set& before, const se_ptr& after) {
  z3::expr hbs = z3.mk_true();
  for( auto& bf : before ) {
      hbs = hbs && mk_hbs( bf, after );
  }
  return hbs;
}

z3::expr encoding::mk_hbs(const se_ptr& before, const se_set& after) {
  z3::expr hbs = z3.mk_true();
  for( auto& af : after ) {
      hbs = hbs && mk_hbs( before, af );
  }
  return hbs;
}


z3::expr encoding::mk_hbs(const se_set& before, const se_set& after) {
  z3::expr hbs = z3.mk_true();
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

hb encoding::mk_hb_c11_mo(const se_ptr& before, const se_ptr& after) {
  return make_hb( before->get_c11_mo_stamp(), after->get_c11_mo_stamp() );
}

hb encoding::mk_hb_c11_hb(const se_ptr& before, const se_ptr& after) {
  return make_hb_po( before->get_c11_hb_stamp(), after->get_c11_hb_stamp() );
}

hb encoding::mk_hb_c11_hb_ao(const se_ptr& before, const se_ptr& after) {
  return make_hb_po_ao( before->get_c11_hb_stamp(), after->get_c11_hb_stamp() );
}

hb encoding::mk_hb_c11_sc(const se_ptr& before, const se_ptr& after) {
  return make_hb( before->get_c11_sc_stamp(), after->get_c11_sc_stamp() );
}


  z3::expr encoding::mk_guarded_forbid_expr( const hb_enc::hb_vec& vec ) {
    z3::expr hbs = z3.mk_true();
    for( auto& hb : vec ) {
      hbs = hbs && hb->get_guarded_forbid_expr();
    }
    return hbs;
  }

  z3::expr encoding::mk_expr( const hb_enc::hb_vec& vec ) {
    z3::expr hbs = z3.mk_true();
    for( auto& hb : vec ) {
      hbs = hbs && *hb;
    }
    return hbs;
  }

bool encoding::eval_hb( const z3::model& m,
                        const se_ptr& before, const se_ptr& after ) const{
  return eval_hb( m, before->e_v, after->e_v );
}


  z3::expr encoding::get_rf_bit( const tara::variable& v1,
                                 const se_ptr& wr,
                                 const se_ptr& rd ) const {
    assert( ( wr->is_pre() || wr->is_wr() ) &&
            ( rd->is_rd() || rd->is_post() ) );
    assert( !wr->is_wr() || v1 == wr->prog_v );
    assert( !rd->is_rd() || v1 == rd->prog_v );

    std::string bname = v1+"-"+wr->name()+"-"+rd->name();
    return z3.c.bool_const(  bname.c_str() );
  }

  z3::expr encoding::get_rf_bit( const se_ptr& wr,
                                 const se_ptr& rd ) const {
    assert( wr->is_wr() && rd->is_rd() );
    assert( wr->prog_v == rd->prog_v );
    const tara::variable& v1 = wr->prog_v;
    return get_rf_bit( v1, wr, rd );
    // std::string bname = v1+"-"+wr->name()+"-"+rd->name();
    // return z3.c.bool_const(  bname.c_str() );
  }


vector<hb_ptr> encoding::get_hbs( z3::model& m ) {
  vector<hb_ptr> result;
  //todo: how does the following code knows to read from which solver?
  // the following looks in the context, not in solver ??
  z3::expr_vector asserted = z3.c.collect_last_asserted_linear_constr();
  z3::expr_vector asserted_po = z3.c.collect_last_asserted_po_constr();

  for( unsigned i = 0; i<asserted.size(); i++ ) {
    z3::expr atom = asserted[i];
    auto hb = get_hb( atom );
    if( hb && !hb->loc1->special && !hb->loc2->special &&
        hb->loc1->thread != hb->loc2->thread ) {
      if( hb->e1 && hb->e2 ) { // for compatibility of the earlier version of tara
        z3::expr v1 = m.eval( hb->e1->guard );
        z3::expr v2 = m.eval( hb->e2->guard );
        if( Z3_get_bool_value( v1.ctx(), v1)  != Z3_L_TRUE ||
            Z3_get_bool_value( v2.ctx(), v2)  != Z3_L_TRUE ) {
          continue;
        }
      }
      assert(eval_hb(m, hb->loc1, hb->loc2));
      hb_ptr h = make_shared<hb_enc::hb>(*hb);
      result.push_back(h);
    }
  }
  for( unsigned i = 0; i < asserted_po.size(); i++ ) {
    z3::expr atom = asserted_po[i];
    // std::cout << atom << std::endl;
    auto hb = get_hb( atom );
    if( hb && !hb->loc1->special && !hb->loc2->special &&
        hb->loc1->thread != hb->loc2->thread ) {
      if( hb->e1 && hb->e2 ) { // for compatibility of the earlier version of tara
        z3::expr v1 = m.eval( hb->e1->guard );
        z3::expr v2 = m.eval( hb->e2->guard );
        if( Z3_get_bool_value( v1.ctx(), v1)  != Z3_L_TRUE ||
            Z3_get_bool_value( v2.ctx(), v2)  != Z3_L_TRUE ) {
          continue;
        }
      }
      hb_ptr h = make_shared<hb_enc::hb>(*hb);
      result.push_back(h);
      // std::cerr << atom << "\n";
    }

  }

  // current_rf_map.clear();
  //get rf hbs
  for( auto& it : rf_map ) {
    std::string bname = std::get<0>(it);
    z3::expr b = z3.c.bool_const( bname.c_str() );
    z3::expr bv = m.eval( b );
    if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
      hb_enc::se_ptr wr = std::get<1>(it);
      hb_enc::se_ptr rd = std::get<2>(it);
      // z3::expr bp = z3::implies(wr->guard && rd->guard, b );
      // z3::expr bp = z3::implies(wr->guard, b );
      z3::expr bp = b;
      hb_ptr h = make_shared<hb_enc::hb>(wr, rd, bp, false, hb_t::rf);
      result.push_back( h );
      current_rf_map.insert( std::make_pair( bname, h ) );
    }
  }

  return result;
}

}}
