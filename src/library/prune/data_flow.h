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

#ifndef TARA_PRUNE_DATA_FLOW_H
#define TARA_PRUNE_DATA_FLOW_H

#include "prune/prune_base.h"
#include "input/ctrc_program.h"

namespace tara {
namespace prune {
class data_flow : public prune_base
{
public:
  data_flow(const helpers::z3interf& z3, const tara::program& program);
  virtual std::list< z3::expr > prune(const std::list< z3::expr >& hbs, const z3::model& m);
  //todo : remove above and implement befow!!
  virtual hb_enc::hb_vec prune( const hb_enc::hb_vec& hbs, const z3::model& m );

  virtual std::string name();
private:
  const cssa::program* program_old;
  /**
   * @brief Checks which one of the disjuncts is true, then inserts only that one.
   */
  void insert_disjunct(const z3::expr& disjunction, std::list< z3::expr >& hbs, const z3::model& m);
};
}}

#endif // TARA_PRUNE_DATA_FLOW_H
