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

// coherence
// (* sc per location *) acyclic po-loc|rf|fr|co

// (* ppo *)
// let dp     = addr|data
// let rdw    = po-loc & (fre;rfe)
// let detour = po-loc & (coe;rfe)
// let ii0    = dp|rdw|rfi
// let ic0    = 0
// let ci0    = (ctrl+isync)|detour
// let cc0    = dp|po-loc|ctrl|(addr;po)

// let rec ii = ii0 | ci     | (ic;ci) | (ii;ii)
// and     ic = ic0 | ii     | cc      | (ic;cc) | (ii;ic)
// and     ci = ci0 | (ci;ii)| (cc;ci)
// and     cc = cc0 | ci     | (ci;ic) |(cc;cc)
// let ppo = RR(ii)|RW(ic)

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
