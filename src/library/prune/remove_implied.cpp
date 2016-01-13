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

#include "remove_implied.h"

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_implied::remove_implied(const z3interf& z3, const cssa::program& program) : prune_base(z3, program)
{
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    throw std::runtime_error("unsupported");
    //create a solver
      
    // push phi_po
    
  }
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
}

string remove_implied::name()
{
  return string("remove_implied");
}

list< z3::expr > remove_implied::prune(const list< z3::expr >& hbs, const z3::model& m)
{
  list<z3::expr> result = hbs;
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    throw std::runtime_error("unsupported");
    return result;
  }
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
  for (auto it = result.begin() ; it != result.end(); ) {
    bool remove = false;
    for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
      // ensure that we do not compare with ourselves
      if (it != it2) {
        // find duplicate
        if ((Z3_ast)*it == (Z3_ast)*it2) {
          remove = true;
          break;
        }
        unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
        unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
        assert (hb1 && hb2);
        // check if these edge is between the same thread
        if (hb1->loc1->thread == hb2->loc1->thread && hb1->loc2->thread == hb2->loc2->thread) {
          // check if the other one is more specific
          if (hb1->loc1->instr_no <= hb2->loc1->instr_no && hb1->loc2->instr_no >= hb2->loc2->instr_no)
          {
            remove = true;
            break;
          }
        }
      }
    }
    if (remove) {
      //cerr << *it << endl;
      it = result.erase(it);
    }else 
      ++it;
  }
  return result;
}

