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
#include "helpers/z3interf.h"
#include <vector>
#include <list>
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

    static bool check_ppo( mm_t, const hb_enc::se_ptr&, const hb_enc::se_ptr& );

  private:
    helpers::z3interf& z3;
    api::options& o;
    hb_enc::encoding& hb_encoding;
    tara::program& p;

    void distinct_events();

    bool is_po( hb_enc::se_ptr x, hb_enc::se_ptr y );

    static bool is_non_mem_ordered(const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    z3::expr is_ordered_dependency(const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    static bool is_ordered_sc     (const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    static bool is_ordered_tso    (const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    static bool is_ordered_pso    (const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    static bool is_ordered_alpha  (const hb_enc::se_ptr&,const hb_enc::se_ptr&);

    void ppo_traverse ( const tara::thread& );

    void ppo_sc ( const tara::thread& );
    bool check_ppo( const hb_enc::se_ptr&, const hb_enc::se_ptr& );
    void ppo();

    // void wmm_test_ppo(); // TODO: remove when confident

    void unsupported_mm() const;

    bool is_no_thin_mm() const;

    bool anti_po_read( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool anti_po_loc_fr( const hb_enc::se_ptr& rd,
                         const hb_enc::se_ptr& wr );

    bool in_grf        ( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool is_rd_rd_coherence_preserved();
    bool is_wr_wr_coherence_needed();
    bool is_rd_wr_coherence_needed();

    // z3::expr wmm_mk_ghb_thin(const hb_enc::se_ptr& bf,const hb_enc::se_ptr& af);
    // z3::expr wmm_mk_ghb     (const hb_enc::se_ptr& bf,const hb_enc::se_ptr& af);

    z3::expr get_rf_bvar( const tara::variable& v1,
                          hb_enc::se_ptr wr, hb_enc::se_ptr rd,
                          bool record=true );

    z3::expr get_rel_seq_bvar( hb_enc::se_ptr wrp, hb_enc::se_ptr wr,
                               bool record=true );

    void ses();

    // -----------------------------------------------------------------------
    // rmo functions
    static bool is_ordered_rmo    (const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    void ppo_rmo_traverse ( const tara::thread& );

    // -----------------------------------------------------------------------
    // c11 functions
    static bool is_ordered_c11(const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    void ppo_c11( const tara::thread& );
    void ses_c11();
    void distinct_events_c11();

    // -----------------------------------------------------------------------
    // arm8_2 functions

    static bool is_ordered_arm8_2(const hb_enc::se_ptr&,const hb_enc::se_ptr&);
    z3::expr is_dependency_arm8_2( const hb_enc::se_ptr& e1,
                                   const hb_enc::se_ptr& e2  );
    void ppo_arm8_2( const tara::thread& );
    z3::expr rfi_ord_arm8_2( z3::expr rf_b, const hb_enc::se_ptr& wr,
                             const hb_enc::se_ptr& rd );
    // void distinct_events_arm8_2();

    // -----------------------------------------------------------------------
    //power data structures and variables

    typedef std::vector<std::tuple<z3::expr,hb_enc::se_ptr,hb_enc::se_ptr>> relation;
    typedef std::pair<hb_enc::se_ptr,hb_enc::se_ptr> event_pair;
    //rec_rel:=map<<event1,event2>,<time,bit,guard>>
    typedef std::map<event_pair,std::tuple<z3::expr,z3::expr,z3::expr>> rec_rel;
    relation rf_rel,fr_rel,coe_rel;
    std::set<event_pair> fence_rel;
    z3::expr fixpoint =z3.mk_true();
    z3::expr ppo_expr =z3.mk_true();
    z3::expr prop_expr=z3.mk_true();
    z3::expr fence    =z3.mk_true();
    std::map<event_pair,z3::expr> ii0,ci0,cc0,hb_rel;
    hb_enc::se_set hb_ev_set1,hb_ev_set2;
    rec_rel hb_star;

    // power functions

    static bool is_ordered_power(const hb_enc::se_ptr&, const hb_enc::se_ptr&);
    void ppo_power( const tara::thread& );
    void fence_power();
    void prop_power();
    void hb_star_power();
    void compute_ppo_by_fpt(const tara::thread& thread,//compute ii, ic, ci, cc
    												const hb_enc::se_set& ev_set1,
														const hb_enc::se_set& ev_set2);
    void get_power_ii0(const tara::thread& thread,
        									 hb_enc::se_set& ev_set1,
    											 hb_enc::se_set& ev_set2);
    void get_power_ci0(const tara::thread& thread,
        									 hb_enc::se_set& ev_set1,
    											 hb_enc::se_set& ev_set2);
    void get_power_cc0(const tara::thread& thread,
        									 hb_enc::se_set& ev_set1,
    											 hb_enc::se_set& ev_set2);
    void initialize(rec_rel& r, const hb_enc::se_ptr& e1,
    								const hb_enc::se_ptr& e2,std::string r_name);
    void initialize_rels(rec_rel& ii, rec_rel& ic,
    		                  rec_rel& ci, rec_rel& cc,
													const hb_enc::se_set& ev_set1,
													const hb_enc::se_set& ev_set2);
    z3::expr derive_from_single(const rec_rel& r,
    														const event_pair& ev_pair,
    														const z3::expr& t);
    void combine_two(const rec_rel& r1, const rec_rel& r2, const hb_enc::se_ptr& e1,
    								 const hb_enc::se_ptr& e2, const hb_enc::se_ptr& eb,
    		             const z3::expr& t, z3::expr& cond);
    void compute_trans_closure(const hb_enc::se_set& ev_set1,
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 const hb_enc::se_set& ev_set2,
															 const std::map<event_pair,z3::expr>& r,
															 rec_rel& r_star, std::string rel_name);

    //------------------------------------------------------------------------

    z3::expr dist = z3.mk_true();
    z3::expr po   = z3.mk_true();
    z3::expr wf   = z3.mk_true();
    z3::expr rf   = z3.mk_true();
    z3::expr grf  = z3.mk_true();
    z3::expr lrf  = z3.mk_true();
    z3::expr fr   = z3.mk_true();
    z3::expr ws   = z3.mk_true();
    z3::expr thin = z3.mk_true();
    z3::expr co   = z3.mk_true();

    //c11 support
    z3::expr rel_seq = z3.mk_true();

    bool is_ordered( hb_enc::se_ptr x, hb_enc::se_ptr y );
    void update_orderings();
    void update_must_before( const hb_enc::se_vec& es, hb_enc::se_ptr e );
    void update_must_after ( const hb_enc::se_vec& es, hb_enc::se_ptr e );
    void update_may_after  ( const hb_enc::se_vec& es, hb_enc::se_ptr e );
    void update_ppo_before ( const hb_enc::se_vec& es, hb_enc::se_ptr e );
    void pointwise_and ( const hb_enc::depends_set&, z3::expr cond,
                         hb_enc::depends_set& );

    // Set for deletion

    // void sc_ppo_old   ( const tara::thread& );
    // void tso_ppo_old  ( const tara::thread& );
    // void pso_ppo_old  ( const tara::thread& );
    // void rmo_ppo_old  ( const tara::thread& );
    // void alpha_ppo_old( const tara::thread& );
    // void power_ppo_old( const tara::thread& );

    // z3::expr insert_tso_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    // z3::expr insert_pso_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    // z3::expr insert_rmo_barrier( const tara::thread&,unsigned, hb_enc::se_ptr );
    // z3::expr insert_barrier( unsigned tid, unsigned instr );

    void old_ses();
    bool anti_ppo_read_old( const hb_enc::se_ptr& wr, const hb_enc::se_ptr& rd );
    bool anti_po_loc_fr_old( const hb_enc::se_ptr& rd, const hb_enc::se_ptr& wr );
  };
}}

#endif // TARA_CSSA_WMM_H
