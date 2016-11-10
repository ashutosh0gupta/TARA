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

#include "diffvar.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

diffvar::diffvar( const z3interf& z3,
                  const tara::program& program ) : prune_base(z3, program)
{
  if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso()
      || program.is_mm_rmo() || program.is_mm_alpha() ) {
  }else{
    throw std::runtime_error("diffvar does not support memory model!");
  }
}

string diffvar::name()
{
  return string("diffvar");
}

hb_enc::hb_vec diffvar::prune( const hb_enc::hb_vec& hbs,
                               const z3::model& m )
{
  assert( program.is_mm_declared() );
  hb_enc::hb_vec result = hbs;
  for (auto it = result.begin() ; it != result.end(); ) {
    shared_ptr<hb_enc::hb> hb1 = *it;
    auto& l1 = hb1->e1;
    auto& l2 = hb1->e2;
    bool remove = false;
    if( l1->is_mem_op() && l2->is_mem_op() && l1->tid != l2->tid ) {
      if( l1->prog_v != l2->prog_v ) remove = true;
      if( l1->is_rd() && l2->is_rd() ) remove = true; // todo: is_it_correct
    }
    // if( !l1->special && !l2->special && l1->thread != l2->thread ) {
    //   if( l1->prog_v_name != l2->prog_v_name ) remove = true;
    //   if( l1->is_read && l2->is_read ) remove = true; // todo: is_it_correct
    // }
    if( remove ) it = result.erase(it); else ++it;
  }

  return result;
}

