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

#include "output_base.h"
#include <stdexcept>

using namespace tara::api::output;
using namespace tara::api;
using namespace std;

output_base::output_base(helpers::z3interf& _z3) : z3(_z3)
{}

output_base::~output_base() 
{}

void output_base::init(const tara::hb_enc::encoding& hb_encoding, const z3::solver& sol_bad, const z3::solver& sol_good, std::shared_ptr< const tara::program > program)
{
  // z3_ctx = sol_bad->ctx();
  _hb_encoding = &hb_encoding;
  this->sol_bad = unique_ptr<z3::solver>(new z3::solver(sol_bad));
  this->sol_good = unique_ptr<z3::solver>(new z3::solver(sol_good));
  this->program = program;
}


void output_base::output(const z3::expr& output)
{
  if (!_hb_encoding) 
    throw logic_error("Object not yet initialised!");
  _output = unique_ptr<z3::expr>(new z3::expr(output));
}


bool output_base::output_ready()
{
  return _output==nullptr;
}

void output_base::ready_or_throw() const
{
  if (!_output) 
    throw logic_error("No output specified yet!");
}

void output_base::gather_statistics(metric& metric) const
{
  // do nothing
}


std::ostream& operator<< (std::ostream& stream, const output_base& out) {
    out.print(stream, false);
    return stream;
}


