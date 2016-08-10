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

#ifndef TARA_CSSA_SYMBOLIC_EVENT_H
#define TARA_CSSA_SYMBOLIC_EVENT_H

#include "constants.h"
#include "helpers/z3interf.h"
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <boost/concept_check.hpp>

namespace tara{
namespace hb_enc {

  class location;

  enum class event_t {
    pre,  // initialization event
      r,    // read events
      w,    // write events
      barr, // barrier events
      barr_b, // only order before events
      barr_a, // only order after events
      post  // final event
      };

  class symbolic_event;
  typedef std::shared_ptr<symbolic_event> se_ptr;
  typedef std::set<se_ptr> se_set;
  typedef std::vector<se_ptr> se_vec;

//
// symbolic event
//
  class symbolic_event
  {
  private:
    std::shared_ptr<tara::hb_enc::location>
    create_internal_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                           std::string& event_name, unsigned tid,
                           unsigned instr_no, bool special, bool is_read,
                           std::string& prog_v_name);
  public:
    symbolic_event( z3::context& ctx, hb_enc::encoding& hb_encoding,
                    unsigned _tid, unsigned i,
                    const cssa::variable& _v, const cssa::variable& _prog_v,
                    std::string loc, event_t _et );

    symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                    unsigned _tid, unsigned instr_no,
                    std::string _loc, event_t _et );

    // symbolic_event( hb_enc::encoding& _hb_enc, z3::context& ctx,
    //                 unsigned instr_no, unsigned thin_tid,
    //                 const cssa::variable& _v, const cssa::variable& _prog_v,
    //                 hb_enc::location_ptr _loc, event_t _et);

  public:
    unsigned tid;
    cssa::variable v;               // variable with ssa name
    cssa::variable prog_v;          // variable name in the program
    std::string loc_name;
  private:
    unsigned topological_order;
  //   hb_enc::location_ptr loc; // location in
  public:
    std::shared_ptr<tara::hb_enc::location> e_v; // variable for solver
    std::shared_ptr<tara::hb_enc::location> thin_v; // thin air variable
    event_t et;
    se_set prev_events; // in straight line programs it will be singleton
                        // we need to remove access to  pointer
    z3::expr guard;
    inline std::string name() const {
      return e_v->name;
    }

    inline bool is_pre()            const { return et == event_t::pre;    }
    inline bool is_rd()             const { return et == event_t::r;      }
    inline bool is_wr()             const { return et == event_t::w;      }
    inline bool is_barrier()        const { return et == event_t::barr;   }
    inline bool is_before_barrier() const { return et == event_t::barr_b; }
    inline bool is_after_barrier () const { return et == event_t::barr_a; }
    inline bool is_barr_type()      const {
      return is_barrier() || is_before_barrier() || is_after_barrier();
    }

    inline bool is_post()           const { return et == event_t::post;   }
    inline bool is_mem_op()         const { return is_rd() || is_wr();    }

    inline unsigned get_instr_no() const {
      return e_v->instr_no;
    }

    inline unsigned get_topological_order() const {
      return topological_order;
      // return e_v->instr_no;
    }

    inline void set_topological_order( unsigned order ) {
      topological_order = order;
    }

    inline z3::expr get_solver_symbol() const {
      return *e_v;
    }

    inline z3::expr get_thin_solver_symbol() const {
      return *thin_v;
    }

    inline unsigned get_tid() const {
      return tid;
    }

    void set_pre_events( se_set& );

    friend std::ostream& operator<< (std::ostream& stream,
                                     const symbolic_event& var) {
      stream << var.name();
      return stream;
    }
    void debug_print(std::ostream& stream );
    z3::expr get_var_expr(const cssa::variable&);
  };

  typedef std::unordered_map<std::string, se_ptr> name_to_ses_map;

  inline se_ptr mk_se_ptr_old( hb_enc::encoding& _hb_enc,
                               unsigned _tid, unsigned _instr_no,
                               const cssa::variable& _prog_v,
                               std::string _loc,
                               event_t _et,
                               se_set& prev_events) {
    unsigned max = 0;
    for( const se_ptr e : prev_events)
      if( max < e->get_topological_order()) max = e->get_topological_order();

    std::string prefix = _et == hb_enc::event_t::r ? "pi_" : "";
    cssa::variable _n_v = prefix + _prog_v + "#" + _loc;
    // assert( _v == _n_v );
    se_ptr e = std::make_shared<symbolic_event>( _hb_enc.ctx, _hb_enc, _tid, _instr_no,
                                                 _n_v, _prog_v,
                                                 _loc,
                                                 // _loc->name,
                                                 _et );
    e->set_pre_events( prev_events );
    e->set_topological_order( max+1 );
    e->guard = _hb_enc.ctx.bool_val(true);

    // se_store[e->name()] = e;
    return e;
  }


  inline se_ptr mk_se_ptr_old( hb_enc::encoding& _hb_enc,
                               unsigned _tid, unsigned _instr_no,
                               std::string _loc, event_t _et,
                               se_set& prev_events ) {
    unsigned max = 0;
    for( const se_ptr e : prev_events )
      if( max < e->get_topological_order()) max = e->get_topological_order();

    se_ptr e = std::make_shared<symbolic_event>( _hb_enc.ctx, _hb_enc, _tid, _instr_no,
                                                 _loc, _et );
    e->set_pre_events( prev_events );
    e->set_topological_order( max+1 );
    e->guard = _hb_enc.ctx.bool_val(true);
    // se_store[e->name()] = e;
    return e;
  }

  //--------------------------------------------------------------------------
  // new calls
  // todo: streamline se allocation

  inline se_ptr
  mk_se_ptr( hb_enc::encoding& _hb_enc, unsigned _tid, se_set prev_events,
             z3::expr& cond, const cssa::variable& _prog_v, std::string _loc,
             event_t _et ) {
    unsigned max = 0;
    for( const se_ptr e : prev_events)
      if( max < e->get_topological_order()) max = e->get_topological_order();
    cssa::variable ssa_var = _prog_v + "#" + _loc;
    se_ptr e = std::make_shared<symbolic_event>( _hb_enc.ctx, _hb_enc, _tid,
                                                 max+1,
                                                 ssa_var, _prog_v, _loc, _et);
    e->guard = cond;
    e->set_pre_events( prev_events );
    e->set_topological_order( max+1 );
    return e;
  }

  inline se_ptr
  mk_se_ptr( hb_enc::encoding& _hb_enc, unsigned _tid, se_set prev_events,
             z3::expr& cond, std::string _loc, event_t _et ) {
    unsigned max = 0;
    for( const se_ptr e : prev_events)
      if( max < e->get_topological_order() ) max = e->get_topological_order();
    se_ptr e = std::make_shared<symbolic_event>( _hb_enc.ctx, _hb_enc, _tid,
                                                 max+1,
                                                 _loc, _et );
    e->set_pre_events( prev_events );
    e->guard = cond;
    // e->guard = _hb_enc.ctx.bool_val(true);
    e->set_topological_order( max+1 );
    return e;
  }

  //--------------------------------------------------------------------------

  struct se_hash {
    size_t operator () (const se_ptr &v) const {
      return std::hash<std::string>()(v->e_v->name);
    }
  };

  struct se_equal :
    std::binary_function <symbolic_event,symbolic_event,bool> {
    bool operator() (const se_ptr& x, const se_ptr& y) const {
      return std::equal_to<std::string>()(x->e_v->name, y->e_v->name);
    }
  };

  // typedef std::unordered_set<se_ptr, se_hash, se_equal> se_set;

  typedef std::unordered_map<cssa::variable, se_ptr, cssa::variable_hash, cssa::variable_equal> var_to_se_map;

  typedef std::unordered_map<cssa::variable, se_set, cssa::variable_hash, cssa::variable_equal> var_to_ses_map;

  typedef std::unordered_map<cssa::variable, se_vec, cssa::variable_hash, cssa::variable_equal> var_to_se_vec_map;

  typedef std::unordered_map<se_ptr, se_set, se_hash, se_equal> se_to_ses_map;

  class depends{
  public:
    se_ptr e;
    z3::expr cond;
    depends( se_ptr e_, z3::expr cond_ ) : e(e_), cond(cond_) {}
    friend inline bool operator<( const depends& d1, const depends& d2 ) {
      return d1.e < d2.e;
    }
  };

  typedef std::set<depends> depends_set;

  typedef std::unordered_map<se_ptr, depends_set, se_hash, se_equal> se_to_depends_map;
  typedef std::unordered_map< cssa::variable,
                              depends_set,
                              cssa::variable_hash,
                              cssa::variable_equal> var_to_depends_map;


  inline bool is_po( const se_ptr& x, const se_ptr& y ) {
    if( x == y ) return true;
    if( x->tid != y->tid ) return false;
    if( x->e_v->instr_no < y->e_v->instr_no ) return true;
    if( x->e_v->instr_no == y->e_v->instr_no && x->is_rd() && y->is_wr() )
      return true;
    return false;
  }

  bool is_po_new( const se_ptr& x, const se_ptr& y );

  void debug_print_se_set(const se_set& set, std::ostream& out);

}}

#endif // TARA_CSSA_SYMBOLIC_EVENT_H
