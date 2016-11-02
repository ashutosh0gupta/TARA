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

#ifndef TARA_PRUNE_DIFFVAR_H
#define TARA_PRUNE_DIFFVAR_H

#include "prune/prune_base.h"

namespace tara {
namespace prune {
class diffvar : public prune::prune_base
{
public:
  diffvar(const helpers::z3interf& z3, const tara::program& program);
  //todo : remove the code above
  virtual hb_enc::hb_vec prune( const hb_enc::hb_vec& hbs, const z3::model& m );
  virtual std::string name();
private:
};
}}

#endif // PRUNE_DIFFVAR_H
