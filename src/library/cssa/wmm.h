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
 */

#ifndef TARA_CSSA_WMM_H
#define TARA_CSSA_WMM_H

#include "constants.h"
#include "input/program.h"
#include "helpers/z3interf.h"
#include <vector>
#include <list>
#include "cssa/thread.h"
#include <unordered_map>
#include <unordered_set>
#include <boost/concept_check.hpp>

namespace tara{
namespace cssa {

  // class for constructiing constraints on events
  // depending on the memory model
  class wmm_event_cons {
  public:
    wmm_event_cons( helpers::z3interf&, hb_enc::encoding&, cssa::program&  );
    void run();
  private:
    helpers::z3interf& z3;
    hb_enc::encoding& hb_encoding;
    cssa::program& p;

    void wmm_mk_distinct_events();

    void wmm_build_sc_ppo   ( const thread& thread );
    void wmm_build_tso_ppo  ( const thread& thread );
    void wmm_build_pso_ppo  ( const thread& thread );
    void wmm_build_rmo_ppo  ( const thread& thread );
    void wmm_build_alpha_ppo( const thread& thread );
    void wmm_build_power_ppo( const thread& thread );
    void wmm_build_ppo();
    void wmm_test_ppo(); // TODO: remove when confident

    void unsupported_mm() const;


  };

}}

#endif // TARA_CSSA_WMM_H
