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
#include "hb_enc/hb.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_implied::remove_implied(const z3interf& z3, const tara::program& program) : prune_base(z3, program)
{
  if( program.is_mm_declared() ) {
    if( program.is_mm_sc()
        || program.is_mm_tso()
        || program.is_mm_pso()
        || program.is_mm_rmo()
        || program.is_mm_alpha()
        || program.is_mm_c11()
        || program.is_mm_arm8_2()
        ){
    }else{
      throw std::runtime_error("remove_implied does not support memory model!");
    }
  }
}

string remove_implied::name()
{
  return string("remove_implied");
}

bool remove_implied::must_happen_after( const hb_enc::se_ptr e1,
                                        const hb_enc::se_ptr e2 ) {
  if( e1 == e2 ) return true;
  if( e1->get_tid() != e2->get_tid() ) return false;
  // e1 must be ordered before e2 in order to "must occur after"
  if( e1->get_topological_order() > e2->get_topological_order() )
    return false;

  return helpers::exists( program.must_after.at(e1), e2 );
}

bool remove_implied::must_happen_before( const hb_enc::se_ptr e1,
                                         const hb_enc::se_ptr e2 ) {
  if( e1 == e2 ) return true;
  if( e1->get_tid() != e2->get_tid() ) return false;
  // e1 must be ordered before e2 in order to "must occur before"
  if( e1->get_topological_order() > e2->get_topological_order() )
    return false;

  return helpers::exists( program.must_before.at(e2), e1 );
}


hb_enc::hb_vec remove_implied::prune( const hb_enc::hb_vec& hbs,
                                      const z3::model& m )
{
   hb_enc::hb_vec result = hbs;
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    for (auto it = result.begin() ; it != result.end(); ) {
      bool remove = false;
      for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
        // ensure that we do not compare with ourselves
        if( it != it2 ) {
          auto& hb1 = *it;
          auto& hb2 = *it2;
          assert( hb1 ); assert( hb2 );
          assert( hb1->e1 && hb1->e2 && hb2->e1 && hb2->e2 );
          if( must_happen_before( hb1->e1, hb2->e1 ) &&
              must_happen_after( hb2->e2, hb1->e2 ) ) {
            remove = true;
            break;
          }
        }
      }
      if (remove) {
        it = result.erase(it);
      }else 
        ++it;
    }
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
        auto& hb1 = *it;
        auto& hb2 = *it2;
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

