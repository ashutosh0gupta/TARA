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

#ifndef TARA_PRUNE_PRUNE_FUNCTIONS_H
#define TARA_PRUNE_PRUNE_FUNCTIONS_H

#include "prune/prune_base.h"

namespace tara {
namespace prune {
  
  typedef std::vector<std::unique_ptr<prune_base>> prune_chain;
  
  z3::expr apply_prune_chain(const tara::prune::prune_chain& prune_chain, std::list< z3::expr >& hbs, const z3::model& m, unsigned int print_pruning, std::ostream& print_out, const hb_enc::encoding& hb_encoding);
  bool build_prune_chain(const std::string& prune_order, std::vector<std::unique_ptr<prune_base>>& prune_chain, const helpers::z3interf& z3, const cssa::program& program, z3::solver sol_good);

}
}

#endif // TARA_PRUNE_PRUNE_FUNCTIONS_H
