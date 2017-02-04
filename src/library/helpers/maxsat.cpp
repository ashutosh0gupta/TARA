/*
 * Copyright 2017, TIFR
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

#include "z3interf.h"

using namespace std;
using namespace tara;
using namespace tara::helpers;

void z3interf::assert_soft_constraints( z3::solver&s ,
                                        std::vector<z3::expr>& cnstrs,
                                        std::vector<z3::expr>& aux_vars ) {
  for( auto f : cnstrs ) {
    auto n = get_fresh_bool();
    aux_vars.push_back(n);
    s.add( f || n ) ;
  }
}

z3::expr z3interf:: at_most_one( z3::expr_vector& vars ) {
  z3::expr result = vars.ctx().bool_val(true);
  if( vars.size() <= 1 ) return result;
  // todo check for size 0
  z3::expr last_xor = vars[0];
  for( unsigned i = 1; i < vars.size(); i++ ) {
    z3::expr curr_xor = (vars[i] != last_xor );
    result = result && (!last_xor || curr_xor);
    last_xor = curr_xor;
  }

  return result;
}

int z3interf:: fu_malik_maxsat_step( z3::solver &s,
                                     std::vector<z3::expr>& soft_cnstrs,
                                     std::vector<z3::expr>& aux_vars ) {
    z3::expr_vector assumptions(c);
    z3::expr_vector core(c);
    for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
      assumptions.push_back(!aux_vars[i]);
    }
    if (s.check(assumptions) != z3::check_result::unsat) {
      return 1; // done
    }else {
      core=s.unsat_core();
      // std::cout << core.size() << "\n";
      z3::expr_vector block_vars(c);
      // update soft-constraints and aux_vars
      for (unsigned i = 0; i < soft_cnstrs.size(); i++) {
        unsigned j;
        // check whether assumption[i] is in the core or not
        for( j = 0; j < core.size(); j++ ) {
          if( assumptions[i] == core[j] )
            break;
        }
        if (j < core.size()) {
          z3::expr block_var = get_fresh_bool();
          z3::expr new_aux_var = get_fresh_bool();
          soft_cnstrs[i] = ( soft_cnstrs[i] || block_var );
          aux_vars[i]    = new_aux_var;
          block_vars.push_back( block_var );
          s.add( soft_cnstrs[i] || new_aux_var );
        }
      }
      z3::expr at_most_1 = at_most_one( block_vars );
      s.add( at_most_1 );
      return 0; // not done.
    }
}

z3::model z3interf::fu_malik_maxsat( z3::expr hard,
                                     std::vector<z3::expr>& soft ) {
    unsigned k;
    z3::solver s(c);
    s.add( hard );
    assert( s.check() != z3::unsat );
    assert( soft.size() > 0 );

    std::vector<z3::expr> aux_vars;
    assert_soft_constraints( s, soft, aux_vars );
    k = 0;
    for (;;) {
      if( fu_malik_maxsat_step(s, soft, aux_vars) ) {
    	  z3::model m=s.get_model();
    	  return m;
      }
      k++;
    }
}

// #define Z3_MAXSAT 0
#define Z3_MAXSAT 1

z3::model z3interf::maxsat( z3::expr hard, std::vector<z3::expr>& soft ) {

  if( Z3_MAXSAT ) {
    auto opt =  z3::optimize(c);
    opt.add( hard );
    for( auto& se : soft ) opt.add( se, 1 );
    opt.check();
    if(0) {
      std::cout << opt.help();
      std::cout << opt.statistics();
    }
    return opt.get_model();
  }else
    return fu_malik_maxsat( hard, soft );
}
