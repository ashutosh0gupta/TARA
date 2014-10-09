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

#ifndef TARA_API_METRIC_H
#define TARA_API_METRIC_H
#include <ostream>

namespace tara {
namespace api {

class metric
{
public:
  std::string filename;
  long long unsigned rounds = 0;
  long long unsigned time = 0; // milliseconds
  unsigned instructions = 0;
  unsigned threads = 0;
  unsigned shared_reads = 0;
  unsigned sum_reads_from = 0;
  long long unsigned disjuncts = 0;
  long long unsigned sum_hbs = 0;
  long long unsigned disjuncts_after = 0;
  long long unsigned sum_hbs_after = 0;
  long long unsigned sum_round_time = 0; // microseconds
  
  long long unsigned additional_time = 0; // microseconds
  
  std::string additional_info;
  
  bool unsat_core = false;
  bool data_flow = false;
  friend std::ostream& operator<< (std::ostream& stream, const metric& metric);
  void print(std::ostream& stream, bool latex = false, bool interrupted = false) const;
};
}
}

#endif // TARA_API_METRIC_H
