/*
 * Copyright 2017, TIFR
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
 */

#include "input/ctrc_input.h"
#include "input/parse.h"
#include "input/ctrc_program.h"
// #include "helpers/helpers.h"

using namespace tara;
using namespace tara::ctrc_input;

ctrc::program* tara::ctrc_input::parse_ctrc_file( helpers::z3interf& z3_,
                                                  api::options& _o,
                                                  hb_enc::encoding& hb_encoding,
                                                  // input::program& pa,
                                                  std::string& ctrc_file ) {
  input::program pa = input::parse::parseFile( ctrc_file.c_str() );
  if( _o.mm != mm_t::none ) {
    pa.set_mm( _o.mm );
  }
  pa.convert_instructions( z3_, hb_encoding );
  pa.check_correctness();

  return new ctrc::program( z3_, _o, hb_encoding, pa );
}
