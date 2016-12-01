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

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

unsat_core::unsat_core(const z3interf& z3, const tara::program& program, z3::solver sol_good) : prune_base(z3, program), sol_good(sol_good)
{}

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

hb_enc::hb_vec unsat_core::prune( const hb_enc::hb_vec& hbs,
                                  const z3::model& m)
{
  std::list< z3::expr > hbs_expr;
    // test minunsat here
  sol_good.push();
  // insert initial variable valuation
  z3::expr initial = program.get_initial(m);
  unsigned i = 0;
  for( auto& hb : hbs ) {
    i++;
    z3::expr h_expr = (*hb);
    hbs_expr.push_back( h_expr );
    if( hb->e1 && hb->e2 ) {
      z3::expr e = hb->e1->guard && hb->e2->guard;
      // std::cout << i << " " << hb->e1->name() << hb->e2->name() << " " << e << "\n";
      //sol_good.add(e);
    }
  }

  sol_good.add(program.get_initial(m));
  z3interf::min_unsat<z3::expr>(sol_good, hbs_expr, [](z3::expr e) { return e; });
  // { // in return state is sat then we can debug the state
  //   z3::model mp = sol_good.get_model();
  //   program.print_execution( "dump", mp );
  // }
  sol_good.pop();
  hb_enc::hb_vec final_result;
  for( z3::expr e : hbs_expr ) {
    auto u_hb = hb_enc.get_hb( e );
    hb_enc::hb_ptr hb = make_shared<hb_enc::hb>( *u_hb );
    final_result.push_back( hb );
  }
  return final_result;
}
