/*
 * Copyright 2016, TIFR
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

#ifndef TARA_API_HB_H
#define TARA_API_HB_H

#include <vector>
#include <memory>
#include <z3++.h>
#include <unordered_map>
#include <set>

#include "../helpers/helpers.h"
#include "helpers/z3interf.h"

namespace tara {
namespace hb_enc {


  // In memory model, there may be several kind of timing constraints
  enum class hb_t {
     hb   // timing ordering                  // sc in c11
   , rf   // rf ordering
   , phb  // partial ordered hb introduced in // base in c11
   , thin // thin air hb                      // mo in c11
       };

struct hb {
public:
  se_ptr e1;
  se_ptr e2;
  tstamp_ptr loc1; //todo : to be removed; type should contain all the needed info
  tstamp_ptr loc2; //todo : to be removed
  bool is_neg;
  bool is_partial;
  hb_t type;
  operator z3::expr () const;
  z3::expr get_guarded_forbid_expr();
  hb(se_ptr loc1, se_ptr loc2, z3::expr expr);
  hb(tstamp_ptr loc1, tstamp_ptr loc2, z3::expr expr);
  hb( se_ptr e1, tstamp_ptr loc1,
      se_ptr e2, tstamp_ptr loc2, z3::expr expr, bool is_neg );
  hb( se_ptr e1, tstamp_ptr loc1,
      se_ptr e2, tstamp_ptr loc2,
      z3::expr expr, bool is_neg, bool is_partial );
  hb( se_ptr e1_, se_ptr e2_, z3::expr expr, bool is_neg, hb_t type_ );
  uint32_t signature(); // a unique integer indentifying the hb

  bool operator==(const hb &other) const;
  bool operator!=(const hb &other) const;

  friend std::ostream& operator<< (std::ostream& stream, const hb& hb);
  void debug_print(std::ostream& stream );

  hb negate() const;

  bool is_hb()   const { return type == hb_t::hb; };
  bool is_rf()   const { return type == hb_t::rf; };
  bool is_partial_ord_hb() const { return type == hb_t::phb; };
  bool is_thin() const { return type == hb_t::thin; };

  friend bool operator< (const hb& hb1, const hb& hb2);
private:
  z3::expr expr;
  uint32_t _signature = 0;
  void update_signature();
};

}
  void debug_print( std::ostream& out, const hb_enc::hb_vec& );

} // end namespaces

#endif // TARA_API_ENCODING_H
