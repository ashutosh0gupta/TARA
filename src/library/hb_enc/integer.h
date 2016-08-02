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
#include "hb_enc/symbolic_event.h"

namespace tara {
namespace hb_enc {
  class integer : public hb_enc::encoding
  {
  public:
    integer(z3::context& ctx);
    virtual void make_event( se_ptr locations ) override;
    virtual void make_location( std::shared_ptr< location > locations ) override;
    virtual void make_locations(std::vector< std::shared_ptr< location > > locations) override;
    virtual hb_enc::hb make_hb(location_ptr loc1, location_ptr loc2) const override;
    virtual hb_enc::as make_as(location_ptr loc1, location_ptr loc2) const override;
    virtual bool eval_hb(const z3::model& model, location_ptr loc1, location_ptr loc2) const override;
    virtual std::unique_ptr<hb_enc::hb> get_hb(const z3::expr& hb, bool allow_equal = false) const override;
    virtual std::vector<location_ptr> get_trace(const z3::model& m) const override;
  protected:
    std::unordered_map<z3::expr, std::shared_ptr<hb_enc::location>> location_lookup;
    std::unordered_map<z3::expr, se_ptr > event_lookup;
  // map location to instruction
    typedef std::unordered_map<z3::expr, std::shared_ptr<hb_enc::location>>::const_iterator mapit;
    std::pair<mapit,mapit> get_locs(const z3::expr& hb, bool& possibly_equal) const;
    
    // z3::context& ctx;
  };
}}

#endif // TARA_HB_ENC_INTEGER_H
