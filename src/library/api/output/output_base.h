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

#ifndef TARA_API_OUTPUT_BASE_H
#define TARA_API_OUTPUT_BASE_H

#include <string>
#include <z3++.h>
#include "hb_enc/encoding.h"
#include "program/program.h"
#include <memory>
#include "api/metric.h"

namespace tara {
namespace api {
namespace output {
class output_base
{
public:
  output_base(helpers::z3interf& _z3);
  virtual ~output_base();
  virtual void print(std::ostream& stream, bool machine_readable) const = 0;

  virtual void init(const tara::hb_enc::encoding& hb_encoding, const z3::solver& sol_bad, const z3::solver& sol_good, std::shared_ptr< const tara::program > program);
  virtual void output(const z3::expr& output);
  virtual void gather_statistics(metric& metric) const;
  bool output_ready();
protected:
  helpers::z3interf& z3;
  const hb_enc::encoding* _hb_encoding = nullptr;
  std::unique_ptr<z3::expr> _output = nullptr;
  std::unique_ptr<z3::solver> sol_bad = nullptr;
  std::unique_ptr<z3::solver> sol_good = nullptr;
  std::shared_ptr<const tara::program> program = nullptr;
  void ready_or_throw() const;
  
  
  
  /***************************
   * Useful utility functions
   ***************************/
  inline const tara::instruction& lookup(const hb_enc::tstamp_ptr& loc) const; // lookup a location in the program
  /**
   * @brief Check if hb.loc1 is reading and hb.loc2 is writing to the same variable
   * 
   * @param hb ...
   * @return Set of variables in common between these location (empty if none)
   */
  inline cssa::variable_set read_write(const hb_enc::hb& hb) const; // check if hb.loc1 is reading and hb.loc2 is writing
  inline cssa::variable_set write_read(const hb_enc::hb& hb) const; // check if hb.loc1 is writing and hb.loc2 is reading
  inline cssa::variable_set write_write(const hb_enc::hb& hb) const; // check if hb.loc1 is writing and hb.loc2 is writing
  inline cssa::variable_set read_read(const hb_enc::hb& hb) const; // check if hb.loc1 is reading and hb.loc2 is reading the same variable
  inline cssa::variable_set access_access(const hb_enc::hb& hb) const; // check if hb.loc1 and hb.loc2 use the same variable
  inline bool same_inverted(const hb_enc::hb& hb1, const hb_enc::hb& hb2) const; // check if the hbs are exactly inverted of each other
  inline bool order_hb(hb_enc::hb& hb1, hb_enc::hb& hb2) const; // order the two hb in a way that hb1 -> hb2, returns false if this is not possible
  
  /**
   * @brief Orders the two locations if they are from the same thread
   */
  inline void order_locations(hb_enc::tstamp_ptr& loc1, hb_enc::tstamp_ptr& loc2);
  
  /**
   * @brief Order the locations so that the first location points to the thread with the lower number
   */
  inline void order_locations_thread(hb_enc::tstamp_ptr& loc1, hb_enc::tstamp_ptr& loc2);
  
  /**
   * @brief Checks if two pairs of locations overlap
   */
  inline bool check_overlap(const hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2);
  
  /**
   * @brief Checks if one pair is contained in the other
   */
  inline bool check_contained(const hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2);
  
  /**
   * @brief Merge into the existing locs1 the second locs, so that they cover both intervals
   */
  inline void merge_overlap(hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2);
private:
};
}}}

std::ostream& operator<< (std::ostream& stream, const tara::api::output::output_base& out);

#endif // TARA_API_OUTPUT_BASE_H
