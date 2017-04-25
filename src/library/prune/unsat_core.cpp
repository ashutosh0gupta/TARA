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

#include "unsat_core.h"
#include "helpers/z3interf_templates.h"
#include "hb_enc/hb.h"

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

unsat_core::unsat_core(const z3interf& z3,
                       const tara::program& program,
                       z3::solver sol_good)
  : prune_base(z3, program), sol_good(sol_good), rf_scc_finder( z3 )
{
  rf_scc_finder.program = &program;
  rf_scc_finder.verbose = program.options().print_pruning;
}

string unsat_core::name()
{
  return string("unsat_core");
}


list< z3::expr > unsat_core::prune(const list< z3::expr >& hbs, const z3::model& m)
{
  // test minunsat here
  sol_good.push();
  // insert initial variable valuation
  z3::expr initial = program.get_initial(m);
  sol_good.add(program.get_initial(m));
  list< z3::expr > hbsn = hbs;
  z3interf::min_unsat<z3::expr>(sol_good, hbsn, [](z3::expr e) { return e; });
  sol_good.pop();
  return hbsn;
}

// return the set of rf edges that form thin-air cycle
bool unsat_core::find_thin_air_cycles( const hb_enc::hb_vec& hbs,
                                       const z3::model& m,
                                       hb_enc::hb_vec& involved_rfs) {
  std::vector< hb_enc::se_set > sccs;
  hb_enc::hb_vec rf_hbs;
  involved_rfs.clear();
  for( auto& hb : hbs ) {
    if( hb->type == hb_enc::hb_t::rf || hb->type == hb_enc::hb_t::hb ||
        ( hb->type == hb_enc::hb_t::phb && !hb->is_neg) ) {
      auto& wr = hb->e1;
      auto& rd = hb->e2;
      if( wr == NULL || rd == NULL ) continue; // pre POPL'15 case
      if(  wr->is_wr() && rd->is_rd() && wr->access_same_var(rd)) {
        auto b = hb_enc.get_rf_bit( wr, rd );
        z3::expr bv = m.eval( b );
        if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
          rf_hbs.push_back( hb );
        }
      }
    }
  }
  if( rf_hbs.size() == 0 ) return false;
  rf_scc_finder.find_sccs(rf_hbs, sccs);
  //todo : inefficient code; prticipating rfs can be computed while calculating
  //       sccs; now we
  bool thin_found = false;
  for( auto& scc : sccs ) {
    if( scc.size() > 1 ) {
      thin_found = true;
      //found an rf cycle
      for( auto& hb : rf_hbs ) {
        if( helpers::exists( scc, hb->e1) && helpers::exists( scc, hb->e2 ) ) {
          involved_rfs.push_back(hb);
        }
      }
    }
  }
  return thin_found;
}

hb_enc::hb_vec unsat_core::prune( const hb_enc::hb_vec& hbs,
                                  const z3::model& m)
{
  std::list< z3::expr > hbs_expr;
  std::list< z3::expr > rfs_expr;
  // test minunsat here
  sol_good.push();
  // insert initial variable valuation
  z3::expr initial = program.get_initial(m);
  unsigned i = 0;
  for( auto& hb : hbs ) {
    i++;
    z3::expr h_expr = (*hb);
    if( hb->type == hb_enc::hb_t::rf ) {
      z3::expr gurd = hb->e1->guard && hb->e2->guard;
      h_expr = z3::implies( gurd, h_expr );
    }
    hbs_expr.push_back( h_expr );
  }
  sol_good.add(program.get_initial(m));

  auto id_fun = [](z3::expr e) { return e; };
  bool is_unsat = z3interf::min_unsat<z3::expr>(sol_good,hbs_expr,id_fun,true );

#ifndef NDEBUG
  if( !is_unsat && program.options().print_pruning > 3 ) {
    prune_error( "untested_code!!" );
    sol_good.push();
    for( auto& hb :  hbs_expr ) sol_good.add( hb );
    auto res = sol_good.check();
    assert( res == z3::sat );
    auto mp = sol_good.get_model();
    program.print_execution( "unwanted-good", mp );
    sol_good.pop();
  }
#endif // NDEBUG

  hb_enc::hb_vec thin_air_rfs;
  if( !is_unsat ) {
    // rf cycles are suspected
    if( find_thin_air_cycles( hbs, m, thin_air_rfs ) ) {
      hb_enc::se_set rds;
      z3::expr fix_thin = z3.mk_true();
      for( auto rf : thin_air_rfs ) {
        hb_enc::se_ptr rd = rf->e1;
        z3::expr vname = rd->rd_v();
        z3::expr e = m.eval(vname);
        if (((Z3_ast)vname) != ((Z3_ast)e))
          fix_thin = fix_thin && vname == e;
      }
      sol_good.add( fix_thin );
      if( program.options().print_pruning > 2 ) {
        debug_print( program.options().out(), thin_air_rfs );
      }
    }else{
      prune_error( "thin air values found!!" );
    }
    z3interf::min_unsat<z3::expr>( sol_good, hbs_expr, id_fun );
  }
  sol_good.pop();
  hb_enc::hb_vec final_result;
  for( z3::expr e : hbs_expr ) {
    if( z3interf::is_implies( e ) ) {
      auto u_hb = hb_enc.get_hb( e.arg(1) );
      final_result.push_back( u_hb );
    }else{
      auto u_hb = hb_enc.get_hb( e );
      // hb_enc::hb_ptr hb = make_shared<hb_enc::hb>( *u_hb );
      final_result.push_back( u_hb );
    }
  }
  vec_insert( final_result, thin_air_rfs );
  helpers::remove_duplicates( final_result );
  return final_result;
}
