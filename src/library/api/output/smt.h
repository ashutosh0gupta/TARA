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

#ifndef TARA_API_OUTPUT_SMT_H
#define TARA_API_OUTPUT_SMT_H

#include "api/output/output_base.h"

namespace tara {
namespace api {
namespace output {

class smt : public output_base
{
public:
  smt(options& o_, helpers::z3interf& _z3) : output_base(o_, _z3) {};
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  using output_base::output;
  virtual z3::expr output() const;
};
}
}
}

#endif // TARA_API_OUTPUT_SMT_H
