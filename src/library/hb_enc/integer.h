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

#ifndef TARA_HB_ENC_INTEGER_H
#define TARA_HB_ENC_INTEGER_H

#include "hb_enc/encoding.h"
#include <unordered_map>
#include "helpers/helpers.h"
#include "helpers/z3interf.h"
#include "hb_enc/symbolic_event.h"
#include "hb_enc/hb.h"

namespace tara {
namespace hb_enc {
  class integer : public hb_enc::encoding
  {
  public:
    integer(helpers::z3interf& z3);
    // integer(z3::context& ctx);
    virtual void record_event( se_ptr& e ) override;
    virtual void make_tstamp( tstamp_var_ptr ) override;
    virtual void add_time_stamps(std::vector< tstamp_var_ptr > ) override;
    virtual hb_enc::hb make_hb(tstamp_ptr loc1, tstamp_ptr loc2) const override;
    virtual hb_enc::hb make_hb_po(tstamp_ptr loc1, tstamp_ptr loc2) const override;
    virtual hb_enc::as make_as(tstamp_ptr loc1, tstamp_ptr loc2) const override;
    virtual bool eval_hb(const z3::model& model, tstamp_ptr loc1, tstamp_ptr loc2) const override;
    virtual hb_ptr get_hb(const z3::expr& hb, bool allow_equal = false) const override;
    virtual std::vector<tstamp_ptr> get_trace(const z3::model& m) const override;
    void make_po_tstamp( tstamp_var_ptr );
  protected:
    std::unordered_map< z3::expr, tstamp_var_ptr > tstamp_lookup;
    std::unordered_map< z3::expr, se_ptr, helpers::z3interf::expr_hash, helpers::z3interf::expr_equal > event_lookup;
  // map tstamp to instruction
    typedef std::unordered_map<z3::expr, tstamp_var_ptr>::const_iterator mapit;
    std::pair<mapit,mapit> get_locs(const z3::expr& hb, bool& possibly_equal, bool& is_partial) const;
  private:
    // todo: should the following be static??
    uint16_t counter = 0;
    z3::sort hb_sort = z3.mk_sort( "HB" );

  };
}}

#endif // TARA_HB_ENC_INTEGER_H
