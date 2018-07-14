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
 *
 */

#include "wmm.h"
#include "helpers/helpers.h"
#include "helpers/z3interf.h"
#include "hb_enc/hb.h"
#include <assert.h>

using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;


// com = rf | fr | co

// coherence
// (* sc per location *) acyclic po-loc | rf | fr | co

// (* fences *)
// let fence = RM(lwsync)|WW(lwsync)|sync

// (* no thin air *)
// let hb = ppo|fence|rfe
// acyclic hb

// (* prop *)
// let prop-base = (fence|(rfe;fence));hb*
// let prop = WW(prop-base)|(com*;prop-base*;sync;hb*)

// (* observation *) irreflexive fre;prop;hb*

// (* propagation *) acyclic co | prop

bool wmm_event_cons::is_ordered_power( const hb_enc::se_ptr& e1,
                                       const hb_enc::se_ptr& e2  ) {
  assert(false);
}

void wmm_event_cons::ppo_power( const tara::thread& thread ) {
  // (* ppo *)
  // let ii0    = addr | data | ( po-loc & (fre;rfe) ) |rfi
  // let ic0    = 0
  // let ci0    = (ctrl+isync)|( po-loc & (coe;rfe) )
  // let cc0    = addr| data| po-loc|ctrl|(addr;po)

  // let rec ii = ii0 | ci      | (ic;ci) | (ii;ii)
  // and     ic = ic0 | ii | cc | (ic;cc) | (ii;ic)
  // and     ci = ci0           | (ci;ii) | (cc;ci)
  // and     cc = cc0 | ci      | (ci;ic) | (cc;cc)
  // let ppo = RR(ii)|RW(ic)
  hb_enc->mk_ghb( e1, e2 );
  z3::expr prop_e1_e2 = hb_enc->mk_ghb_power_prop( e1, e2 );
  
  p.unsupported_mm();
}

