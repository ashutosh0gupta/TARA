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

#ifndef TARA_PRUNE_BASE_H
#define TARA_PRUNE_BASE_H

#include "cssa/program.h"
#include <list>

namespace tara {
namespace prune {
class prune_base 
{
public:
  virtual ~prune_base();
  virtual std::string name() = 0;
  virtual std::list< z3::expr > prune(const std::list< z3::expr >& hbs, const z3::model& m) = 0;
  // todo: remove the above call and remove the following const in 1st param
  virtual hb_enc::hb_vec prune( const hb_enc::hb_vec& hbs, const z3::model& m) = 0;//{ return hbs; };
protected:
  prune_base(const helpers::z3interf& z3, const tara::program& program);
  const helpers::z3interf& z3;
  const tara::program& program;
  const hb_enc::encoding& hb_enc;
public:  
  friend z3::expr apply_prune_chain(const std::vector<std::unique_ptr<prune_base>>& prunes, const std::list< z3::expr >& hbs, const z3::model& m); // forward declaration
};

}}

#endif // TARA_PRUNE_BASE_H
