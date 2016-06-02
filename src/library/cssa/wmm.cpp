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

#include "program.h"
#include "cssa_exception.h"
#include "helpers/helpers.h"
#include "helpers/int_to_string.h"
#include <string.h>
#include "helpers/z3interf.h"
#include <utility>
#include <assert.h>
using namespace tara;
using namespace tara::cssa;
using namespace tara::helpers;

using namespace std;

//----------------------------------------------------------------------------
// New code
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// wmm configuration
//----------------------------------------------------------------------------

bool program::is_mm_declared() const
{
  return mm != mm_t::none;
}

bool program::is_wmm() const
{
  return mm != mm_t::sc;
}

bool program::is_mm_sc() const
{
  return mm == mm_t::sc;
}

bool program::is_mm_tso() const
{
  return mm == mm_t::tso;
}

bool program::is_mm_pso() const
{
  return mm == mm_t::pso;
}

bool program::is_mm_rmo() const
{
  return mm == mm_t::rmo;
}

bool program::is_mm_alpha() const
{
  return mm == mm_t::alpha;
}

bool program::is_mm_power() const
{
  return mm == mm_t::power;
}

void program::set_mm(mm_t _mm)
{
  mm = _mm;
}

mm_t program::get_mm() const
{
  return mm;
}


//----------------------------------------------------------------------------
// utilities for symbolic events

symbolic_event::symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                                unsigned _tid, unsigned instr_no,
                                const variable& _v, const variable& _prog_v,
                                hb_enc::location_ptr _loc, event_kind_t _et )
  : tid(_tid)
  , v(_v)
  , prog_v( _prog_v )
  , loc(_loc)
  , et( _et )
  , guard(ctx)
{
  if( et != event_kind_t::r &&  event_kind_t::w != et ) {
    throw cssa_exception("symboic event with wrong parameters!");
  }
  // bool special = (event_kind_t::i == et || event_kind_t::f == et);
  bool is_read = (et == event_kind_t::r); // || event_kind_t::f == et);
  std::string et_name = is_read ? "R" : "W";
  std::string event_name = et_name + "#" + v.name;
  e_v = make_shared<hb_enc::location>( ctx, event_name, false);
  e_v->thread = _tid;
  e_v->instr_no = instr_no;
  e_v->is_read = is_read;
  e_v->prog_v_name = prog_v.name;
  std::vector< std::shared_ptr<tara::hb_enc::location> > locations;
  locations.push_back( e_v );
  _hb_enc.make_locations(locations);
}

// barrier events
symbolic_event::symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                                unsigned _tid, unsigned instr_no,
                                hb_enc::location_ptr _loc, event_kind_t _et )
  : tid(_tid)
  , v("dummy",ctx)
  , prog_v( "dummy",ctx)
  , loc(_loc)
  , et( _et )
  , guard(ctx)
{
  std::string event_name;
  switch( et ) {
  case event_kind_t::barr: { event_name = "#barr";  break; }
  case event_kind_t::pre : { event_name = "#pre" ;  break; }
  case event_kind_t::post: { event_name = "#post";  break; }
  default: cssa_exception("unreachable code!!");
  }
  event_name = loc->name+event_name;
  e_v = make_shared<hb_enc::location>( ctx, event_name, true);
  e_v->thread = _tid;
  e_v->instr_no = instr_no;
  e_v->is_read = (event_kind_t::post == et);
  e_v->prog_v_name = prog_v.name;
  std::vector< std::shared_ptr<tara::hb_enc::location> > locations;
  locations.push_back( e_v );
  _hb_enc.make_locations(locations);
}

z3::expr symbolic_event::get_var_expr( const variable& g ) {
  assert( et != event_kind_t::barr);
  if( et == event_kind_t::r || et == event_kind_t::w) {
    z3::expr v_expr = (z3::expr)(v);
    return v_expr;
  }
  variable tmp_v(g.sort.ctx());
  switch( et ) {
  // case event_kind_t::barr: { tmp_v = g+"#barr";  break; }
  case event_kind_t::pre : { tmp_v = g+"#pre" ;  break; }
  case event_kind_t::post: { tmp_v = g+"#post";  break; }
  default: cssa_exception("unreachable code!!");
  }
  return (z3::expr)(tmp_v);
}

void symbolic_event::debug_print(std::ostream& stream ) {
  stream << *this << "\n";
  if( et == event_kind_t::r || et == event_kind_t::w ) {
    std::string s = et == event_kind_t::r ? "Read" : "Write";
    stream << s << " var: " << v << ", orig_var : " <<prog_v
           << ", loc :" << loc << "\n";
    stream << "guard: " << guard << "\n";
  }
}

void tara::cssa::debug_print_se_set(const se_set& set, std::ostream& out) {
  for (se_ptr c : set) {
    out << *c << " ";
    }
  out << std::endl;
}

//----------------------------------------------------------------------------
// hb utilities

bool program::hb_eval( const z3::model& m,
                       const cssa::se_ptr& before, const cssa::se_ptr& after ) const{
  return _hb_encoding.eval_hb( m, before->e_v, after->e_v );
}

z3::expr program::wmm_mk_hb(const cssa::se_ptr& before,
                             const cssa::se_ptr& after) {
  return _hb_encoding.make_hb( before->e_v, after->e_v );
}

z3::expr program::wmm_mk_hbs(const cssa::se_ptr& before,
                             const cssa::se_ptr& after) {
  return wmm_mk_hb( before, after );
}

z3::expr program::wmm_mk_hbs(const cssa::se_set& before,
                             const cssa::se_ptr& after) {
  z3::expr hbs = _z3.c.bool_val(true);
  for( auto& bf : before ) {
      hbs = hbs && wmm_mk_hb( bf, after );
  }
  return hbs;
}

z3::expr program::wmm_mk_hbs(const cssa::se_ptr& before,
                             const cssa::se_set& after) {
  z3::expr hbs = _z3.c.bool_val(true);
  for( auto& af : after ) {
      hbs = hbs && wmm_mk_hb( before, af );
  }
  return hbs;
}


z3::expr program::wmm_mk_hbs(const cssa::se_set& before,
                             const cssa::se_set& after) {
  z3::expr hbs = _z3.c.bool_val(true);
  for( auto& bf : before ) {
    for( auto& af : after ) {
      hbs = hbs && wmm_mk_hb( bf, af );
    }
  }
  return hbs;
}

// ghb = guarded hb
z3::expr program::wmm_mk_ghb( const cssa::se_ptr& before,
                              const cssa::se_ptr& after ) {
  return implies( before->guard && after->guard, wmm_mk_hb( before, after ) );
}

//----------------------------------------------------------------------------
// memory model utilities

void program::unsupported_mm() const {
  std::string msg = "unsupported memory model: " + string_of_mm( mm ) + "!!";
  throw cssa_exception( msg.c_str() );
}

bool program::in_grf( const cssa::se_ptr& wr, const cssa::se_ptr& rd ) {
  if( is_mm_sc() ) {
    return true;
  }else if( is_mm_tso() || is_mm_pso() || is_mm_rmo() || is_mm_alpha() ) {
    return wr->tid != rd->tid; //only events with diff threads are part of grf
  }else{
    unsupported_mm();
    return false;
  }
}

bool program::has_barrier_in_range( unsigned tid, unsigned start_inst_num,
                                    unsigned end_inst_num ) const {
  const thread& thread = *threads[tid];
  for(unsigned i = start_inst_num; i <= end_inst_num; i++ ) {
    if( is_barrier( thread[i].type ) ) return true;
  }
  return false;
}

//----------------------------------------------------------------------------
// coherence preserving code
// po-loc must be compatible with
// n - nothing to do
// r - relaxed
// u - not visible
// (pp-loc)/(ses)
//                     sc   tso   pso   rmo
//  - ws      (w->w)   n/n  n/n   n/n   n/n
//  - fr      (r->w)   n/n  r/n   r/n   r/n
//  - fr;rf   (r->r)   n/n  n/u   n/u   r/u
//  - rf      (w->r)   n/n  n/u   n/u   n/u
//  - ws;rf   (w->r)   n/n  n/u   n/u   n/u

bool program::anti_ppo_read( const cssa::se_ptr& wr, const cssa::se_ptr& rd ) {
  // preventing coherence violation - rf
  // (if rf is local then may not visible to global ordering)
  assert( wr->tid == threads.size() ||
          rd->tid == threads.size() ||
          wr->prog_v.name == rd->prog_v.name );
  if( is_mm_sc() || is_mm_tso() || is_mm_pso() || is_mm_rmo() || is_mm_alpha()) {
    // should come here for those memory models where rd-wr on
    // same variables are in ppo
    if( wr->tid == rd->tid && wr->e_v->instr_no >= rd->e_v->instr_no ) {
      return true;
    }
    //
  }else{
    unsupported_mm();
  }
  return false;
}

bool program::anti_po_loc_fr( const cssa::se_ptr& rd, const cssa::se_ptr& wr ) {
  // preventing coherence violation - fr;
  // (if rf is local then may not visible to global ordering)
  // coherance disallows rf(rd,wr') and ws(wr',wr) and po-loc( wr, rd)
  assert( wr->tid == threads.size() || rd->tid == threads.size() ||
          wr->prog_v.name == rd->prog_v.name );
  if( is_mm_sc() || is_mm_tso() || is_mm_pso() || is_mm_rmo() ) {
    if( wr->tid == rd->tid && rd->e_v->instr_no > wr->e_v->instr_no ) {
      return true;
    }
    //
  }else{
    unsupported_mm();
  }
  return false;
}

bool program::is_rd_rd_coherance_preserved() {
  if( is_mm_sc() || is_mm_tso() || is_mm_pso() || is_mm_alpha()) {
    return true;
  }else if( is_mm_rmo() ){
    return false;
  }else{
    unsupported_mm();
    return false; // dummy return
  }
}

//----------------------------------------------------------------------------
// Build symbolic event structure
// In original implementation this part of constraints are
// referred as pi constraints

z3::expr program::get_rf_bvar( const variable& v1, se_ptr wr, se_ptr rd,
                               bool record ) {
  std::string bname = v1+"-"+wr->name()+"-"+rd->name();
  z3::expr b = _z3.c.bool_const(  bname.c_str() );
  if( record ) reading_map.insert( std::make_tuple(bname,wr,rd) );
  return b;
}

void program::wmm_build_ses() {
  // For each global variable we need to construct
  // - wf  well formed
  // - rf  read from
  // - grf global read from
  // - fr  from read
  // - ws  write serialization

  // z3::context& c = _z3.c;

  for( const variable& v1 : globals ) {
    const auto& rds = rd_events[v1];
    const auto& wrs = wr_events[v1];
    unsigned c_tid = 0;
    se_set tid_rds;
    for( const se_ptr& rd : rds ) {
      z3::expr some_rfs = _z3.c.bool_val(false);
      z3::expr rd_v = rd->get_var_expr(v1);
      if( rd->tid != c_tid ) {
        tid_rds.clear();
        c_tid = rd->tid;
      }
      for( const se_ptr& wr : wrs ) {
        if( anti_ppo_read( wr, rd ) ) continue;
        z3::expr wr_v = wr->get_var_expr( v1 );
        z3::expr b = get_rf_bvar( v1, wr, rd );
        some_rfs = some_rfs || b;
        z3::expr eq = ( rd_v == wr_v );
        // well formed
        wf = wf && implies( b, wr->guard && eq);
        // read from
        z3::expr new_rf = implies( b, wmm_mk_ghb( wr, rd ) );
        rf = rf && new_rf;
        //global read from
        if( in_grf( wr, rd ) ) grf = grf && new_rf;
        // from read
        for( const se_ptr& after_wr : wrs ) {
          if( after_wr->name() != wr->name() ) {
            auto cond = b && wmm_mk_ghb(wr, after_wr) && after_wr->guard;
            if( anti_po_loc_fr( rd, after_wr ) ) {
              fr = fr && !cond;
            }else{
              auto new_fr = wmm_mk_ghb( rd, after_wr );
              if( is_rd_rd_coherance_preserved() ) {
                for( auto before_rd : tid_rds ) {
                  //disable this code for rmo
                  z3::expr anti_coherent_b =
                    get_rf_bvar( v1, after_wr, before_rd, false );
                  new_fr = !anti_coherent_b && new_fr;
                }
              }
              fr = fr && implies( cond , new_fr );
            }
          }
        }
      }
      wf = wf && implies( rd->guard, some_rfs );
      tid_rds.insert( rd );
    }

    // write serialization
    auto it1 = wrs.begin();
    for( ; it1 != wrs.end() ; it1++ ) {
      const se_ptr& wr1 = *it1;
      auto it2 = it1;
      it2++;
      for( ; it2 != wrs.end() ; it2++ ) {
        const se_ptr& wr2 = *it2;
        if( wr1->tid != wr2->tid && // Why this condition?
            !wr1->is_init() && !wr2->is_init() // no initializations
            ) {
          ws = ws && ( wmm_mk_ghb( wr1, wr2 ) || wmm_mk_ghb( wr2, wr1 ) );
        }
      }
    }
  }

}


//----------------------------------------------------------------------------
// declare all events happen at different time points

void program::wmm_mk_distinct_events() {

  z3::expr_vector loc_vars(_z3.c);

  for( auto& init_se : init_loc ) {
    loc_vars.push_back( *(init_se->e_v) );
  }

  for( unsigned t=0; t < threads.size(); t++ ) {
    thread& thread = *threads[t];
    assert( thread.size() > 0 );
    for( unsigned j=0; j < thread.size(); j++ ) {
      for( auto& rd : thread[j].rds ) {
        loc_vars.push_back( *(rd->e_v) );
      }
      for( auto& wr : thread[j].wrs ) {
        loc_vars.push_back( *(wr->e_v) );
      }
    }
  }

  phi_distinct = distinct(loc_vars);

}

//----------------------------------------------------------------------------
// build preserved programming order(ppo)

void program::wmm_build_sc_ppo( thread& thread ) {
  unsigned tsize = thread.size();
  se_set last = init_loc;

  for( unsigned j = 0; j < tsize; j++ ) {
    auto& rds = thread[j].rds, wrs = thread[j].wrs;
    if( rds.empty() && wrs.empty() ) continue;
    auto& after = rds.empty() ? wrs : rds;
    phi_po = phi_po && wmm_mk_hbs( last, after ) && wmm_mk_hbs( rds, wrs );
    last = wrs.empty() ? rds : wrs;
  }
  phi_po = phi_po && wmm_mk_hbs( last, post_loc);
}

void program::wmm_build_tso_ppo( thread& thread ) {
  se_set last_rds = init_loc, last_wrs = init_loc;
  bool rd_occured = false;
  se_set barr_events = init_loc;
  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier( thread[j].type ) ) {
      phi_po = phi_po && wmm_mk_hbs( last_rds, thread[j].barr );
      phi_po = phi_po && wmm_mk_hbs( last_wrs, thread[j].barr );
      last_rds = last_wrs = thread[j].barr;
      rd_occured = false;
    }else{
      // if(!is_barrier( thread[j].type )){
      auto& rds = thread[j].rds, wrs = thread[j].wrs;
      if( !rds.empty() ) {
        phi_po = phi_po && wmm_mk_hbs( last_rds, rds );
        last_rds = rds;
        rd_occured = true;
      }
      if( !wrs.empty() ) {
        phi_po = phi_po && wmm_mk_hbs( last_wrs, wrs );
        last_wrs = wrs;

        if( rd_occured ) {
          phi_po = phi_po && wmm_mk_hbs(last_rds, wrs);
        }
      }
    }
  }
  phi_po = phi_po && wmm_mk_hbs(last_wrs, post_loc);
}

void program::wmm_build_pso_ppo( thread& thread ) {
  var_to_se_map last_wr;
  assert( init_loc.size() == 1);
  se_ptr init_l = *init_loc.begin();
  for( auto& g : globals ) last_wr[g] = init_l;

  bool no_rd_occurred = true;
  se_set last_rds = init_loc;
  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier(thread[j].type) ) {
      phi_po = phi_po && wmm_mk_hbs( last_rds, thread[j].barr );
      last_rds = thread[j].barr;
      no_rd_occurred = true;
      assert( thread[j].barr.size() == 1 );
      se_ptr barr = *thread[j].barr.begin();
      for( auto& g : globals ) {
        phi_po = phi_po && wmm_mk_hbs( last_wr[g], barr );
        last_wr[g] = barr;
      }
    }else{
      auto& rds = thread[j].rds;
      auto& wrs = thread[j].wrs;
      if( rds.size() > 0 ) {
        phi_po = phi_po && wmm_mk_hbs( last_rds, rds );
        last_rds = rds;
        no_rd_occurred = false;
      }

      for( auto wr : wrs ) {
        const variable& v = wr->prog_v;
        phi_po = phi_po && wmm_mk_hb( last_wr[v], wr );
        last_wr[v] = wr;
        if( !no_rd_occurred ) {
          phi_po = phi_po && wmm_mk_hbs( last_rds, wr );
        }
      }
    }
  }
  // only writes must be ordered with post event
  for( auto& g : globals ) {
    phi_po = phi_po && wmm_mk_hbs( last_wr[g], post_loc );
  }
  //phi_po = phi_po && barriers;
}

void program::wmm_build_rmo_ppo( thread& thread ) {
  var_to_se_map last_rd, last_wr;
  se_set collected_rds;

  assert( init_loc.size() == 1);
  se_ptr barr = *init_loc.begin();
  for( auto& g : globals ) last_rd[g] = last_wr[g] = barr;

  for( unsigned j=0; j<thread.size(); j++ ) {
    if( is_barrier( thread[j].type ) ) {
      assert( thread[j].barr.size() == 1);
      barr = *thread[j].barr.begin();
      for( se_ptr rd : collected_rds) {
        phi_po = phi_po && wmm_mk_hbs( rd, barr );
      }
      collected_rds.clear();

      for( auto& g : globals ) {
        phi_po = phi_po && wmm_mk_hbs( last_wr[g], barr );
        last_rd[g] = last_wr[g] = barr;
      }
    }else{
      for( auto rd : thread[j].rds ) {
        phi_po = phi_po && wmm_mk_hbs( barr, rd );
        last_rd[rd->prog_v]  = rd;
        collected_rds.insert( rd );
      }

      for( auto wr : thread[j].wrs ) {
        variable v = wr->prog_v;

        phi_po = phi_po && wmm_mk_hb( last_wr[v], wr );
        phi_po = phi_po && wmm_mk_hb( last_rd[v], wr );
        collected_rds.erase( last_rd[v] );// optimization??

        for( auto rd : dependency_relation[wr] )
          phi_po = phi_po && wmm_mk_hb( rd, wr );

        last_wr[v] = wr;
      }
    }
  }

  for( auto& g : globals )
    phi_po = phi_po && wmm_mk_hbs( last_wr[g], post_loc );
}

void program::wmm_build_alpha_ppo( thread& thread ) {
    var_to_se_map last_wr, last_rd;
    assert( init_loc.size() == 1);
    se_ptr barr = *init_loc.begin();
    for( auto& g : globals ) last_rd[g] = last_wr[g] = barr;

    for( unsigned j=0; j<thread.size(); j++ ) {
        if( is_barrier( thread[j].type ) ) {
	    assert( thread[j].barr.size() == 1);
            barr = *thread[j].barr.begin();
	    for( auto& g : globals ) {
		phi_po = phi_po && wmm_mk_hbs( last_rd[g], barr );
		phi_po = phi_po && wmm_mk_hbs( last_wr[g], barr );
		last_rd[g] = last_wr[g] = barr;
	    }
    }else{
	for( auto rd : thread[j].rds ){
	    const variable& v = rd->prog_v;
	    phi_po = phi_po && wmm_mk_hb( last_rd[v], rd ); //read-read to same loc
	    last_rd[v] = rd;
     }
	for( auto wr : thread[j].wrs ){
	    const variable& v = wr->prog_v;
	    phi_po = phi_po && wmm_mk_hb( last_wr[v], wr ); //write-write to same loc
            phi_po = phi_po && wmm_mk_hb( last_rd[v], wr ); //read-write to same loc
	    last_wr[v] = wr;
      }
   }
}
    for( auto& g : globals )
	phi_po = phi_po && wmm_mk_hbs( last_wr[g], post_loc );
}
/**
// Old implementation of alpha model
void program::wmm_build_alpha_ppo( thread& thread ) {
  // implemented but doubtful that is correct!!
  unsupported_mm();
  //todo: post to be supported

  var_to_se_map last_wr, last_rd;

  for( unsigned j=0; j<thread.size(); j++ ) {
    auto& rds = thread[j].rds;
    auto& wrs = thread[j].wrs;
    for( const variable& v : globals ) {
      se_ptr rd, wr;
      for( auto rd1 : rds ) { if( v == rd1->prog_v ) { rd = rd1; break; } }
      for( auto wr1 : wrs ) { if( v == wr1->prog_v ) { wr = wr1; break; } }
      if( rd ) {
        if( last_rd[v] ) {
          phi_po = phi_po && wmm_mk_hb( last_rd[v], rd );
        }else{
          phi_po = phi_po && wmm_mk_hbs( init_loc, rd );
        }
        last_rd[v] = rd;
      }
      if( wr ) {
        if( last_wr[v] ) {
          phi_po = phi_po && wmm_mk_hb( last_wr[v], wr );
        }else{
          phi_po = phi_po && wmm_mk_hbs( init_loc, wr );
        }
        last_wr[v] = wr;
        if( last_rd[v] ) {
          phi_po = phi_po && wmm_mk_hb( last_rd[v], wr );
        }
      }
    }
  }
}
**/

void program::wmm_build_power_ppo( thread& thread ) {
  unsupported_mm();
}

void program::wmm_build_ppo() {
  // wmm_test_ppo();

  for( unsigned t=0; t<threads.size(); t++ ) {
    thread& thread = *threads[t];
    if( is_mm_sc() )          { wmm_build_sc_ppo ( thread );
    }else if( is_mm_tso() )   { wmm_build_tso_ppo( thread );
    }else if( is_mm_pso() )   { wmm_build_pso_ppo( thread );
    }else if( is_mm_rmo() )   { wmm_build_rmo_ppo( thread );
    }else if( is_mm_alpha() ) { wmm_build_alpha_ppo( thread );
    }else if( is_mm_power() ) { wmm_build_power_ppo( thread );
    }else{                      unsupported_mm();
    }
  }
  phi_po = phi_po && phi_distinct;
  //phi_po = phi_po && barriers;
}

void program::wmm_test_ppo() {
  unsigned t;
  phi_po = _z3.c.bool_val(true);
  for(t=0;t<threads.size();t++){wmm_build_sc_ppo(*threads[t]);}
  std::cerr << "\nsc" << phi_po; phi_po = _z3.c.bool_val(true);
  for(t=0;t<threads.size();t++){wmm_build_tso_ppo(*threads[t]);}
  std::cerr << "\ntso" << phi_po; phi_po = _z3.c.bool_val(true);
  for(t=0;t<threads.size();t++){wmm_build_pso_ppo(*threads[t]);}
  std::cerr << "\npso" << phi_po; phi_po = _z3.c.bool_val(true);
  for(t=0;t<threads.size();t++){wmm_build_rmo_ppo(*threads[t]);}
  std::cerr << "\nrmo" << phi_po; phi_po = _z3.c.bool_val(true);
}

//----------------------------------------------------------------------------
// populate content of threads

void program::wmm_build_cssa_thread(const input::program& input) {

  for( unsigned t=0; t < input.threads.size(); t++ ) {
    thread& thread = *threads[t];
    assert( thread.size() == 0 );

    shared_ptr<hb_enc::location> prev;
    for( unsigned j=0; j < input.threads[t].size(); j++ ) {
      if( shared_ptr<input::instruction_z3> instr =
          dynamic_pointer_cast<input::instruction_z3>(input.threads[t][j]) ) {

        shared_ptr<hb_enc::location> loc = instr->location();
        auto ninstr = make_shared<instruction>( _z3, loc, &thread, instr->name,
                                                instr->type, instr->instr );
        thread.add_instruction(ninstr);
      }else{
        throw cssa_exception("Instruction must be Z3");
      }
    }
  }
}

//----------------------------------------------------------------------------
// encode pre condition of multi-threaded code

void program::wmm_build_pre(const input::program& input) {
  //
  // start location is needed to ensure all locations are mentioned in phi_ppo
  //
  std::shared_ptr<hb_enc::location> _init_l = input.start_name();
  se_ptr wr = mk_se_ptr( _z3.c, _hb_encoding, threads.size(), 0,
                         _init_l, event_kind_t::pre, se_store );
  wr->guard = _z3.c.bool_val(true);
  init_loc.insert(wr);
  for( const variable& v : globals ) {
    variable nname(_z3.c);
    nname = v + "#pre";
    initial_variables.insert(nname);
    wr_events[v].insert( wr );
  }

  if ( shared_ptr<input::instruction_z3> instr =
       dynamic_pointer_cast<input::instruction_z3>(input.precondition)) {
    z3::expr_vector src(_z3.c), dst(_z3.c);
    for( const variable& v: instr->variables() ) {
      variable nname(_z3.c);
      if (!is_primed(v)) {
        nname = v + "#pre";
      } else {
        throw cssa_exception("Primed variable in precondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_pre = instr->instr.substitute(src,dst);
  } else {
    throw cssa_exception("Instruction must be Z3");
  }
}

void program::wmm_build_post(const input::program& input,
                             unordered_map<string, string>& thread_vars ) {
    
  if( shared_ptr<input::instruction_z3> instr =
      dynamic_pointer_cast<input::instruction_z3>(input.postcondition) ) {

    z3::expr tru = _z3.c.bool_val(true);
    if( eq( instr->instr, tru ) ) return;

    std::shared_ptr<hb_enc::location> _end_l = input.end_name();
    se_ptr rd = mk_se_ptr( _z3.c, _hb_encoding, threads.size(), INT_MAX,
                           _end_l, event_kind_t::post, se_store );
    rd->guard = _z3.c.bool_val(true);

    for( const variable& v : globals ) {
      variable nname(_z3.c);
      nname = v + "#post";
      post_loc.insert( rd );
      rd_events[v].push_back( rd );
    }

    z3::expr_vector src(_z3.c), dst(_z3.c);
    for( const variable& v: instr->variables() ) {
      variable nname(_z3.c);
      if( !is_primed(v) ) {
        if( is_global(v) ) {
          nname = v + "#post";
        }else{
          nname = v + "#" + thread_vars[v];
        }
      } else {
        throw cssa_exception("Primed variable in postcondition");
      }
      src.push_back(v);
      dst.push_back(nname);
    }
    // replace variables in expr
    phi_post = instr->instr.substitute(src,dst);
    phi_prp = phi_prp && phi_post;
  } else {
    throw cssa_exception("Instruction must be Z3");
  }
}

//----------------------------------------------------------------------------

z3::expr program::wmm_insert_tso_barrier( thread & thread, unsigned instr,
                                      se_ptr new_barr ) {
  z3::expr hbs = _z3.c.bool_val(true);

  bool before_found = false;
  unsigned j = instr;
  while( j != 0 )  {
    j--;
    if( !thread[j].wrs.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].wrs, new_barr );
      before_found = true;
      break;
    }
    if( !before_found && !thread[j].rds.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].rds, new_barr );
      before_found = true;
    }
  }

  bool after_found = false;
  for(j = instr; j < thread.size(); j++ ) {
    if( !thread[j].rds.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].rds, new_barr );
      after_found = true;
      break;
    }
    if( !after_found && !thread[j].wrs.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].wrs, new_barr );
      after_found = true;
    }
  }
  if( before_found && after_found )  return hbs;
  return _z3.c.bool_val(true);
}

z3::expr program::wmm_insert_pso_barrier( thread & thread, unsigned instr,
                                      se_ptr new_barr ) {
  //todo stop at barriers
  z3::expr hbs = _z3.c.bool_val(true);

  variable_set found_wrs = globals;
  bool before_found = false;
  unsigned j = instr;
  while( j != 0 )  {
    j--;
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && wmm_mk_hb( wr, new_barr );
        found_wrs.erase( it );
        before_found = true;
      }
    }
    if( found_wrs.empty() ) break;
    if( !before_found && !thread[j].rds.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].rds, new_barr );
      before_found = true;
    }
  }

  found_wrs = globals;
  bool after_found = false;
  for( j = instr; j < thread.size(); j++ )  {
    if( !thread[j].rds.empty() ) {
      hbs = hbs && wmm_mk_hbs( thread[j].rds, new_barr );
      after_found = true;
      break;
    }
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && wmm_mk_hb( wr, new_barr );
        found_wrs.erase( it );
        after_found = true;
      }
    }
  }

  if( before_found && after_found )  return hbs;
  return _z3.c.bool_val(true);
}

z3::expr program::wmm_insert_rmo_barrier( thread & thread, unsigned instr,
                                      se_ptr new_barr ) {
  z3::expr hbs = _z3.c.bool_val(true);

  bool before_found = false;
  unsigned j = instr;
  se_set collected_rds;
  variable_set found_wrs = globals;
  while( j != 0 )  {
    j--;
    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && wmm_mk_hb( wr, new_barr );
        found_wrs.erase( it );
        before_found = true;
      }
      for( auto rd : dependency_relation[wr] ) {
        collected_rds.insert( rd );
      }
    }

    for( auto rd: thread[j].rds ) {
      auto it = collected_rds.find(rd);
      if( it != collected_rds.end() ) {
        collected_rds.erase( it );
      }else{
        hbs = hbs && wmm_mk_hb( rd, new_barr );
        before_found = true;
      }
    }
  }

  bool after_found = false;
  found_wrs = globals;
  for( unsigned j = instr; j < thread.size(); j++ ) {

    for( auto rd : thread[j].rds ) {
      const variable& v = rd->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end() ) {
        hbs = hbs && wmm_mk_hb( new_barr, rd );
        after_found = true;
        found_wrs.erase( it );
      }
    }

    for( auto wr: thread[j].wrs ) {
      const variable& v = wr->prog_v;
      auto it = found_wrs.find(v);
      if( it != found_wrs.end()
          // && dependency_relation[wr].empty() // todo: optimization
          ) {
        hbs = hbs && wmm_mk_hb( new_barr, wr );
        found_wrs.erase( it );
        after_found = true;
      }
    }
    if( found_wrs.empty() ) break;
  }

  if( before_found && after_found )  return hbs;
  return _z3.c.bool_val(true);
}

z3::expr program::wmm_insert_barrier(unsigned tid, unsigned instr) {
  assert( tid < threads.size() );
  thread & thread= *threads[instr];
  assert( instr < thread.size() );

  //todo : prepage contraints for barrier
  se_ptr new_barr = mk_se_ptr( _z3.c, _hb_encoding, tid, instr, thread[instr].loc,
                               event_kind_t::barr, se_store );
  z3::expr new_ord(_z3.c);
  if(is_mm_tso()) {
    new_ord = wmm_insert_tso_barrier( thread, instr, new_barr );
  }else if(is_mm_pso()) {
    new_ord = wmm_insert_pso_barrier( thread, instr, new_barr );
  }else if(is_mm_rmo()) {
    new_ord = wmm_insert_rmo_barrier( thread, instr, new_barr );
  }else{
    unsupported_mm();
  }
  return new_ord;
}

void program::wmm_build_ssa( const input::program& input ) {

  wmm_build_pre( input );

  var_to_ses_map dep_events;
  z3::context& c = _z3.c;

  unordered_map<string, string> thread_vars;

  for( unsigned t=0; t < input.threads.size(); t++ ) {
    // last reference to global variable in this thread (if any)
    unordered_map<string, shared_ptr<instruction> > global_in_thread;
    // maps variable to the last place they were assigned

    // set all local vars to "pre"
    for (string v: input.threads[t].locals) {
      thread_vars[ threads[t]->name + "." + v ] = "pre";
    }

    z3::expr path_constraint = c.bool_val(true);
    variable_set path_constraint_variables;
    thread& thread = *threads[t];

    for ( unsigned i=0; i<input.threads[t].size(); i++ ) {
      if ( shared_ptr<input::instruction_z3> instr =
           dynamic_pointer_cast<input::instruction_z3>(input.threads[t][i]) ) {
        z3::expr_vector src(c), dst(c);
        se_set dep_ses;

        // Construct ssa/symbolic events for all the read variables
        for( const variable& v1: instr->variables() ) {
          if( !is_primed(v1) ) {
            variable v = v1;
            variable nname(c);
            src.push_back(v);
            //unprimmed case
            if ( is_global(v) ) {
              nname =  "pi_"+ v + "#" + thread[i].loc->name;
              se_ptr rd = mk_se_ptr( c, _hb_encoding, t, i, nname, v,
                                     thread[i].loc, event_kind_t::r,se_store);
              thread[i].rds.insert( rd );
              rd_events[v].push_back( rd );
              dep_ses.insert( rd );
            }else{
              v = thread.name + "." + v;
              nname = v + "#" + thread_vars[v];
              // check if we are reading an initial value
              if (thread_vars[v] == "pre") { initial_variables.insert(nname); }
              dep_ses.insert( dep_events[v].begin(), dep_events[v].end());
            }
            // the following variable is read by this instruction
            thread[i].variables_read.insert(nname);
            thread[i].variables_read_orig.insert(v);
            dst.push_back(nname);
          }
        }

        // Construct ssa/symbolic events for all the write variables
        for( const variable& v: instr->variables() ) {
          if( is_primed(v) ) {
            src.push_back(v);
            variable nname(c);
            //primmed case
            variable v1 = get_unprimed(v); //converting the primed to unprimed
            if( is_global(v1) ) {
              nname = v1 + "#" + thread[i].loc->name;
              //insert write symbolic event
              se_ptr wr = mk_se_ptr( c,_hb_encoding, t, i, nname,v1,
                                     thread[i].loc, event_kind_t::w,se_store);
              thread[i].wrs.insert( wr );
              wr_events[v1].insert( wr );
              dependency_relation[wr].insert( dep_ses.begin(),dep_ses.end() );
            }else{
              v1 = thread.name + "." + v1;
              nname = v1 + "#" + thread[i].loc->name;
              dep_events[v1].insert( dep_ses.begin(), dep_ses.end() );
            }
            // the following variable is written by this instruction
            thread[i].variables_write.insert( nname );
            thread[i].variables_write_orig.insert( v1 );
            // map the instruction to the variable
            variable_written[nname] = thread.instructions[i];
            dst.push_back(nname);
          }
        }

        // thread havoced variables the same
        for( const variable& v: instr->havok_vars ) {
          assert(false); // untested code
          assert( dep_ses.empty() ); // there must be nothing in dep_ses
          src.push_back(v);
          variable nname(c);
          variable v1 = get_unprimed(v);
          thread[i].havok_vars.insert(v1);
          if( is_global(v1) ) {
            nname = v1 + "#" + thread[i].loc->name;
            se_ptr wr = mk_se_ptr( c, _hb_encoding, t, i, nname, v1,
                                   thread[i].loc,event_kind_t::w, se_store);
            thread[i].wrs.insert( wr );
            wr_events[v1].insert( wr );
            dependency_relation[wr].insert( dep_ses.begin(),dep_ses.end() );
          }else{
            v1 = thread.name + "." + v1;
            nname = v1 + "#" + thread[i].loc->name;
            dep_events[v1].insert(dep_ses.begin(),dep_ses.end());
          }
          // this variable is written by this instruction
          thread[i].variables_write.insert(nname);
          thread[i].variables_write_orig.insert(v1);
          // map the instruction to the variable
          variable_written[nname] = thread.instructions[i];
          dst.push_back(nname);
          // add havok to initial variables
          initial_variables.insert(nname);
        }

        for( se_ptr rd : thread[i].rds ) {
          rd->guard = path_constraint;
        }

        // replace variables in expr
        z3::expr newi = instr->instr.substitute( src, dst );
        // deal with the path-constraint
        thread[i].instr = newi;
        if (thread[i].type == instruction_type::assume) {
          path_constraint = path_constraint && newi;
          path_constraint_variables.insert( thread[i].variables_read.begin(),
                                            thread[i].variables_read.end() );
        }
        thread[i].path_constraint = path_constraint;
        thread[i].variables_read.insert( path_constraint_variables.begin(),
                                         path_constraint_variables.end() );
        if ( instr->type == instruction_type::regular ) {
          phi_vd = phi_vd && newi;
        }else if ( instr->type == instruction_type::assert ) {
          // add this assertion, but protect it with the path-constraint
          phi_prp = phi_prp && implies(path_constraint,newi);
          // add used variables
          assertion_instructions.insert(thread.instructions[i]);
        }
        if( is_barrier( instr->type) ) {
          //todo : prepage contraints for barrier
          se_ptr barr = mk_se_ptr( c, _hb_encoding, t, i,
                                   thread[i].loc, event_kind_t::barr,se_store);
          thread[i].barr.insert( barr );
          tid_to_instr.insert({t,i}); // for shikhar code
        }
        for( se_ptr wr : thread[i].wrs ) {
          wr->guard = path_constraint;
        }

        // increase referenced variables
        for( variable v: thread[i].variables_write_orig ) {
          thread_vars[v] = thread[i].loc->name;
          if ( is_global(v) ) {
            global_in_thread[v] = thread.instructions[i]; // useless??
            thread.global_var_assign[v].push_back(thread.instructions[i]); //useless??
          }
        }
      }else{
        throw cssa_exception("Instruction must be Z3");
      }
    }
    phi_fea = phi_fea && path_constraint; //recordes feasibility of threads
  }
  wmm_build_post( input, thread_vars );
}

void program::wmm( const input::program& input ) {
  
  wmm_build_cssa_thread( input ); // construct thread skeleton
  wmm_build_ssa( input ); // build ssa
  wmm_mk_distinct_events(); // Rd/Wr events on globals are distinct
  wmm_build_ppo(); // build hb formulas to encode the preserved program order
  wmm_build_ses(); // build symbolic event structure
  // wmm_build_barrier(); // build barrier// ppo already has the code

  //TODO: deal with atomic section and prespecified happens befores
  // add atomic sections
  for (input::location_pair as : input.atomic_sections) {
    cerr << "#WARNING: atomic sections are declared!! Not supproted in wmm mode!\n";
    // throw cssa_exception( "atomic sections are not supported!" );
  }
}


//----------------------------------------------------------------------------
// unused code

void program::wmm_add_barrier(int tid, int instr) {
  //assert((threads[tid]->instructions[instr]->type == instruction_type::fence) || (threads[tid]->instructions[instr]->type == instruction_type::barrier));
  thread & thread= *threads[tid];
  auto& barrier_before = thread[instr+1].rds; // everything before (above) is ordered wrt this barrier
  auto& barrier_after = thread[instr].wrs;   // everything after (below) is ordered wrt this barrier
  auto& rds_before = thread[instr].rds;
  auto& rds_after = thread[instr+1].rds;
  auto& wrs_before = thread[instr].wrs;
  auto& wrs_after = thread[instr+1].wrs;

  if( !rds_before.empty() || !wrs_before.empty() ) {
    if( !rds_before.empty() && !wrs_before.empty() ) {
          phi_po = phi_po && wmm_mk_hbs( rds_before , barrier_before );
          phi_po = phi_po && wmm_mk_hbs( wrs_before , barrier_before );
    }else if( !rds_before.empty() ) {
      phi_po = phi_po && wmm_mk_hbs( rds_before , barrier_before );
    } else if( !wrs_before.empty() ) {
      phi_po = phi_po && wmm_mk_hbs( wrs_before , barrier_before );
    }

    for(int j=instr-1; j>=0; j-- ) {
      rds_before = thread[j].rds;
      wrs_before = thread[j].wrs;
      if(!rds_before.empty()) {
        phi_po = phi_po && wmm_mk_hbs( rds_before , barrier_before );
      }
      if(!wrs_before.empty()) {
        phi_po = phi_po && wmm_mk_hbs( wrs_before , barrier_before );
      }
    }
  }

  if( !rds_after.empty() || !wrs_after.empty() ) {
    if( !rds_after.empty() && !wrs_after.empty() ) {
      phi_po = phi_po && wmm_mk_hbs( barrier_after , rds_after );
      phi_po = phi_po && wmm_mk_hbs( barrier_after , wrs_after );
    } else if( !rds_after.empty() ) {
      phi_po = phi_po && wmm_mk_hbs( barrier_after , rds_after );
    } else if( !wrs_after.empty() ) {
      phi_po = phi_po && wmm_mk_hbs( barrier_after , wrs_after );
    }

    for(unsigned i=instr+2; i<thread.size(); i++ ) {
      rds_after = thread[i].rds;
      wrs_after = thread[i].wrs;
      if(!rds_after.empty()) {
        phi_po = phi_po && wmm_mk_hbs( barrier_after , rds_after );
      }
      if(!wrs_after.empty()) {
        phi_po = phi_po && wmm_mk_hbs( barrier_after , wrs_after );
      }
    }
  }
}

void program::wmm_build_barrier() {
  //return;
  int first_read;
  for( auto it1=tid_to_instr.begin();it1!=tid_to_instr.end();it1++) {
    first_read=0;
    thread& thread=*threads[it1->first];
    auto& barrier = thread[it1->second].barr;
    auto& rds_before = thread[it1->second-1].rds;
    auto& rds_after = thread[it1->second+1].rds;
    auto& wrs_before = thread[it1->second-1].wrs;
    auto& wrs_after = thread[it1->second+1].wrs;

    if(is_mm_tso()) {
      if( !wrs_before.empty() ) {
        phi_po = phi_po && wmm_mk_hbs( wrs_before, barrier );
      } else if(!rds_before.empty()) {
        phi_po = phi_po && wmm_mk_hbs( rds_before, barrier );
        for(int j=it1->second-2; j>=0; j-- ) {
          wrs_before = thread[j].wrs;
          if(!wrs_before.empty()) {
            phi_po = phi_po && wmm_mk_hbs( wrs_before, barrier );
            break;
          }
        }
      }
      if( !rds_after.empty() ) {
        phi_po = phi_po && wmm_mk_hbs(barrier, rds_after );
      }
    } else if(!wrs_after.empty()) {
      phi_po = phi_po && wmm_mk_hbs( barrier, wrs_after );

      for( unsigned j=it1->second+2; j!=thread.size(); j++ ) {
        rds_after = thread[j].rds;
        if(!wrs_before.empty()) {
          phi_po = phi_po && wmm_mk_hbs( barrier, rds_after );
          break;
        }
      }
    }
    else if(is_mm_pso()) {
      if( !wrs_before.empty() ) {
        phi_po = phi_po && wmm_mk_hbs( wrs_before, barrier);
      }
      else if(!rds_before.empty()) {
        phi_po = phi_po && wmm_mk_hbs( rds_before, barrier );
        for(int j=it1->second-2; j>=0; j-- ){
          wrs_before = thread[j].wrs;
          if(!wrs_before.empty()){
            phi_po = phi_po && wmm_mk_hbs( wrs_before, barrier );

          }
        }
      }
      if(!wrs_after.empty() || !rds_after.empty()){
        if( !rds_after.empty() && first_read==0 ){
          phi_po = phi_po && wmm_mk_hbs( barrier, rds_after );
          first_read=1;
        }
        if(!wrs_after.empty()){
          phi_po = phi_po && wmm_mk_hbs( barrier, wrs_after );
        }
        for( unsigned i=it1->second+2; i<=thread.size()-1; i++ ){
          rds_after = thread[i].rds;
          wrs_after = thread[i].wrs;
          if(!rds_after.empty() && first_read==0){
            phi_po = phi_po && wmm_mk_hbs( barrier, rds_after );
            first_read=1;
          }
          if(!wrs_after.empty()) {
            phi_po = phi_po && wmm_mk_hbs( barrier, wrs_after );
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
