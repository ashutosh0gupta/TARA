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

#ifndef TARA_PRUNE_REMOVE_IMPLIED_H
#define TARA_PRUNE_REMOVE_IMPLIED_H

#include "prune/prune_base.h"

namespace tara {
namespace prune {
class remove_implied : public prune::prune_base
{
public:
  remove_implied(const helpers::z3interf& z3, const cssa::program& program);
  virtual std::list< z3::expr > prune(const std::list< z3::expr >& hbs, const z3::model& m);
  virtual std::string name();
private:
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  bool compare_sc_events( const hb_enc::location_ptr loc1,
                          const hb_enc::location_ptr loc2 );
  bool compare_tso_events( const hb_enc::location_ptr loc1,
                           const hb_enc::location_ptr loc2 );
  bool compare_pso_events( const hb_enc::location_ptr loc1,
                             const hb_enc::location_ptr loc2 );
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
};
}}

#endif // PRUNE_REMOVE_IMPLIED_H
