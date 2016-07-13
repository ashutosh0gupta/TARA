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

#include "prune_base.h"
#include "api/options.h"

#include "prune/data_flow.h"
#include "prune/remove_implied.h"
#include "prune/unsat_core.h"
#include "prune/remove_thin.h"

namespace tara {
namespace prune {
  
prune_base::prune_base(const helpers::z3interf& z3, const tara::program& program) : z3(z3), program(program), hb_enc(program.hb_encoding())
{

}

prune_base::~prune_base()
{}



}}
