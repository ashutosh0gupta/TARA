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

#include "api/options.h"
#include "constants.h"
#include "program/program.h"
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
    wmm_event_cons( helpers::z3interf&, api::options&,
                    hb_enc::encoding&, tara::program&  );
    void run();
  private:
    helpers::z3interf& z3;
    api::options& o;
    hb_enc::encoding& hb_encoding;
    tara::program& p;

    void distinct_events();

    bool is_barrier_ordered( const hb_enc::se_ptr, const hb_enc::se_ptr );
    bool is_ordered_tso    ( const hb_enc::se_ptr, const hb_enc::se_ptr );
    bool is_ordered_pso    ( const hb_enc::se_ptr, const hb_enc::se_ptr );
    bool is_ordered_rmo    ( const hb_enc::se_ptr, const hb_enc::se_ptr );
    bool is_ordered_alpha  ( const hb_enc::se_ptr, const hb_enc::se_ptr );
    bool is_ordered_power  ( const hb_enc::se_ptr, const hb_enc::se_ptr );

    void new_ppo_sc   ( const tara::thread& );
    void new_ppo_tso  ( const tara::thread& );
    void new_ppo_pso  ( const tara::thread& );
    void new_ppo_rmo  ( const tara::thread& );
    void new_ppo_alpha( const tara::thread& );
    void new_ppo_power( const tara::thread& );

    void sc_ppo   ( const tara::thread& );
    void tso_ppo  ( const tara::thread& );
    void pso_ppo  ( const tara::thread& );
    void rmo_ppo  ( const tara::thread& );
    void alpha_ppo( const tara::thread& );
    void power_ppo( const tara::thread& );
    void ppo();

    void wmm_test_ppo(); // TODO: remove when confident

    void unsupported_mm() const;

    bool anti_ppo_read_new( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool anti_po_loc_fr_new( const hb_enc::se_ptr& rd,
                             const hb_enc::se_ptr& wr );

    bool in_grf        ( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool anti_ppo_read ( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool anti_po_loc_fr( const hb_enc::se_ptr& rd, const hb_enc::se_ptr& wr );
    bool is_rd_rd_coherance_preserved();

    // z3::expr wmm_mk_ghb_thin(const hb_enc::se_ptr& bf,const hb_enc::se_ptr& af);
    // z3::expr wmm_mk_ghb     (const hb_enc::se_ptr& bf,const hb_enc::se_ptr& af);

    z3::expr get_rf_bvar( const variable& v1,
                          hb_enc::se_ptr wr, hb_enc::se_ptr rd,
                          bool record=true );

    void ses();
    void new_ses();

    z3::expr insert_tso_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    z3::expr insert_pso_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    z3::expr insert_rmo_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    z3::expr insert_barrier( unsigned tid, unsigned instr );

    z3::expr dist = z3.mk_true();
    z3::expr po   = z3.mk_true();
    z3::expr wf   = z3.mk_true();
    z3::expr rf   = z3.mk_true();
    z3::expr grf  = z3.mk_true();
    z3::expr fr   = z3.mk_true();
    z3::expr ws   = z3.mk_true();
    z3::expr thin = z3.mk_true();

    bool is_ordered( hb_enc::se_ptr x, hb_enc::se_ptr y );
    void update_orderings();
    void update_must_before( const hb_enc::se_vec& es, hb_enc::se_ptr e );
    void update_must_after ( const hb_enc::se_vec& es, hb_enc::se_ptr e );
  };

}}

#endif // TARA_CSSA_WMM_H
