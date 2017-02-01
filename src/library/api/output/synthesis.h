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

#ifndef TARA_API_OUTPUT_SYNTHESIS_H
#define TARA_API_OUTPUT_SYNTHESIS_H

#include "api/output/nf.h"

namespace tara {
namespace api {
namespace output {
  
typedef std::list<hb_enc::tstamp_pair> lock_locations;

struct lock {
  lock(std::string name);
  std::string name;
  lock_locations locations;
  cssa::variable_set variables;
  friend std::ostream& operator<< (std::ostream& stream, const lock& lock);
};

struct barrier {
  barrier(std::string name);
  std::string name;
  std::unordered_set<hb_enc::tstamp_ptr> locations;
  friend std::ostream& operator<< (std::ostream& stream, const barrier& barrier);
};
  
class synthesis : public output_base
{
public:
  synthesis(options& o_, helpers::z3interf& _z3);
  virtual void print(std::ostream& stream, bool machine_readable) const override;
  virtual void init(const hb_enc::encoding& hb_encoding, const z3::solver& sol_desired, const z3::solver& sol_undesired, std::shared_ptr<const tara::program> program) override;
  virtual void output(const z3::expr& output) override;
  virtual void gather_statistics(metric& metric) const override;
private:
  bool print_nfs;
  nf normal_form;
  std::list<hb_enc::hb> wait_notifies;
  std::list<lock> locks;
  std::list<barrier> barriers;
  void synthesise(nf::result_type& disjunct);
  bool find_lock(const tara::api::output::nf::row_type& disjunct);
  bool find_barrier(const nf::row_type& disjunct, nf::result_type::iterator current, nf::result_type& list);
  /**
   * @brief Merge the locks accross threads.
   */
  void merge_locks_multithread();
  /**
   * @brief Merge the locks within a thread.
   */
  void merge_locks();
  void merge_barriers();
  void merge_barriers_multithread();
  unsigned lock_counter;
  unsigned barrier_counter;
  long long unsigned time;
  std::string info;
};
}
}
}

#endif // TARA_API_OUTPUT_SYNTHESIS_H
