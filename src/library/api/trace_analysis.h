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

#ifndef TARA_API_TRACE_SEPERATION_H
#define TARA_API_TRACE_SEPERATION_H

#include "api/options.h"
#include "api/metric.h"
#include "api/output/output_base.h"
#include "hb_enc/integer.h"

#include <unordered_set>

namespace tara {
  class program;
namespace api {

enum class trace_result {
  always,
  never,
  depends,
  undecided,
};

class trace_analysis
{
public:
  trace_analysis(options& options, helpers::z3interf& _z3);
  void input(std::string input_file);
  void gather_statistics( api::metric& metric );
  trace_result seperate(output::output_base& output, metric& metric);
  bool atomic_sections( std::vector< hb_enc::as >& output, bool merge_as = true);
  bool check_ambigious_traces(std::unordered_set< std::string >& result);
private:
  options _options;
  helpers::z3interf& z3;
  std::shared_ptr<tara::program> program;
  //hb_enc::encoding
  hb_enc::integer hb_encoding;
  /**
   * @brief Creates a solver for good traces
   *
   * @param infeasable Should infeasable traces also be considered good?
   * This is needed for pruning, when we provide inputs
   * @return z3::solver
   */
  z3::solver make_good(bool include_infeasable);
  z3::solver make_bad();
  void connect_read_writes( z3::solver& result );

};
}
}

#endif // TARA_API_TRACE_SEPERATION_H
