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
#include "encoding.h"
#include "program/variable.h"
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <boost/concept_check.hpp>

namespace tara{
namespace hb_enc {

struct tstamp {
private:
  z3::expr expr; // ensure this one is not visible from the outside
  uint16_t _serial;
  
  friend class hb_enc::integer;
public:
  tstamp(tstamp& ) = delete;
  tstamp& operator=(tstamp&) = delete;
  tstamp(z3::context& ctx, std::string name ,bool special = false);

  std::string name;
  /**
   * @brief True if this is a special tstamp
   * 
   * This means the tstamp is not actualy in the program, but the start symbol and the like
   */
  bool special;  //todo: rename to a meaningful name
  int thread; // number of the thread of this tstamp
  int instr_no; // number of this instruction
  /**
   * @brief The previous tstamp in the same thread
   * 
   */
  std::weak_ptr<hb_enc::tstamp const> prev;
  /**
   * @brief The next tstamp in the same thread
   * 
   */
  std::weak_ptr<hb_enc::tstamp const> next;
  
  
  bool operator==(const tstamp &other) const;
  bool operator!=(const tstamp &other) const;
  operator z3::expr () const;
  uint16_t serial() const;
  
  friend std::ostream& operator<< (std::ostream& stream, const tstamp& loc);
  friend std::ostream& operator<< (std::ostream& stream, const tstamp_ptr& loc);
  
  void debug_print(std::ostream& stream );
};

  //todo: the following two enums must be merged

  // C++ specifies ordering tags
  // this notion is not language independent
  enum class o_tag_t {
      na = 0,
      uo = 1,   //unordered
      mon = 2, // monotonic
      acq = 4, // acquire
      rls = 5, // release
      acqrls = 6, //acquire release
      sc = 7, // sequentially consistent
      };

  enum class event_t {
      pre,  // initialization event
      r,    // read events
      w,    // write events
      u,    // update events encodes both successful and failed events
      barr, // barrier events
      barr_b, // only order before events
      barr_a, // only order after events
      block,  // events that indicate the heads of blocks
      post  // final event
      };

  std::string event_t_name( event_t et );

  class symbolic_event;
  typedef symbolic_event se;
  typedef std::shared_ptr<symbolic_event> se_ptr;
  typedef std::set<se_ptr> se_set;
  typedef std::vector<se_ptr> se_vec;
  class depends;
  typedef std::set<depends> depends_set;

//
// symbolic event
//
  class symbolic_event
  {
  private:
    tstamp_var_ptr
    create_internal_event( helpers::z3interf& _hb_enc, std::string event_name,
                           unsigned tid, unsigned instr_no, bool special);
                           // bool is_read, std::string& prog_v_name );
    void update_topological_order();
  public:
    symbolic_event( helpers::z3interf& z3,
                    unsigned _tid, se_set& _prev_events, unsigned i,
                    const tara::variable& _v, const tara::variable& _prog_v,
                    std::string loc, event_t _et );

    symbolic_event( helpers::z3interf& z3,
                    unsigned _tid, se_set& _prev_events, unsigned instr_no,
                    std::string _loc, event_t _et );

  public:
    unsigned tid;
    tara::variable v;            // variable with ssa name
    tara::variable v_copy;       // variable with ssa name
                                 // v_copy is same as v for normal rd and wr
                                 // only for update instruction v_copy is diff
    tara::variable prog_v;       // variable name in the program
    std::string loc_name;
    tara::variable rd_v() { return v; }
    tara::variable wr_v() { return v_copy; }
  private:
    unsigned topological_order;
  public:
    tstamp_var_ptr e_v; // variable for solver
    tstamp_var_ptr thin_v; // thin air variable

    // c11 variables
    tstamp_var_ptr c11_hb_v; // po hb variable

    event_t et;    // type of the event
    o_tag_t o_tag; // ordering tag on the event

    se_set prev_events; // in straight line programs it will be singleton
                        // we need to remove access to  pointer
    hb_enc::depends_set post_events;
    z3::expr guard;
    std::vector<z3::expr> history;
    depends_set data_dependency;
    depends_set ctrl_dependency;

    std::string name() const;
    // inline std::string name() const {
    //   return e_v->name;
    // }

    inline bool is_pre()            const { return et == event_t::pre;    }
    inline bool is_rd()             const { return et == event_t::r ||
                                                   et == event_t::u;      }
    inline bool is_wr()             const { return et == event_t::w ||
                                                   et == event_t::u;      }
    inline bool is_update()         const { return et == event_t::u;      }
    inline bool is_barrier()        const { return et == event_t::barr;   }
    inline bool is_fence()          const { return et == event_t::barr;   }
    inline bool is_before_barrier() const { return et == event_t::barr_b; }
    inline bool is_after_barrier () const { return et == event_t::barr_a; }
    inline bool is_barr_type()      const {
      return is_barrier() || is_before_barrier() || is_after_barrier() ||
        is_block_head ();
    }
    inline bool is_block_head ()    const { return et == event_t::block;  }
    inline bool is_post()           const { return et == event_t::post;   }
    inline bool is_mem_op()         const { return is_rd() || is_wr();    }


    inline bool is_na()     const { return o_tag == o_tag_t::na;  }
    inline bool is_rlx()    const { return o_tag == o_tag_t::mon; }
    inline bool is_rls()    const { return o_tag == o_tag_t::rls; }
    inline bool is_acq()    const { return o_tag == o_tag_t::acq; }
    inline bool is_rlsacq() const { return o_tag == o_tag_t::acqrls; }
    inline bool is_sc()     const { return o_tag == o_tag_t::sc;  }
    inline bool is_atomic() const { return o_tag != o_tag_t::na;  }

    inline bool is_at_least_rls() const {
      // assert( is_wr() || is_pre() );
      return is_rls() || is_rlsacq() || is_sc();
    }

    inline bool is_at_least_acq() const {
      // assert( is_rd() || is_post() );
      return is_acq() || is_rlsacq() || is_sc();
    }

    inline bool is_at_least_rlsacq() const {
      // assert( is_pre() ||
      //         is_wr() || is_rd() || is_post() );
      return is_rlsacq() || is_sc();
    }

    unsigned get_instr_no() const;

    // inline unsigned get_instr_no() const {
    //   return e_v->instr_no;
    // }

    inline unsigned get_topological_order() const {
      return topological_order;
      // return e_v->instr_no;
    }

    inline void set_topological_order( unsigned order ) {
      topological_order = order;
    }

    inline void append_history( z3::expr f ) {
      guard = guard && f;
      history.push_back(f);
    }

    z3::expr get_solver_symbol() const;
    z3::expr get_thin_solver_symbol() const;
    z3::expr get_c11_hb_solver_symbol() const;
    z3::expr get_c11_mo_solver_symbol() const;
    z3::expr get_c11_sc_solver_symbol() const;

    inline bool access_same_var( se_ptr e ) const {
      if( is_pre() || is_post() ) return true;
      return prog_v.name == e->prog_v.name;
    }


    inline tstamp_ptr get_c11_hb_stamp() const {
      return c11_hb_v;
    }

    inline tstamp_ptr get_c11_mo_stamp() const {
      return thin_v;
    }

    inline tstamp_ptr get_c11_sc_stamp() const {
      return e_v;
    }


    inline unsigned get_tid() const {
      return tid;
    }

    void set_pre_events( se_set& );
    void add_post_events( se_ptr&, z3::expr );
    z3::expr get_post_cond( const se_ptr& e_post ) const;

    void set_data_dependency( const hb_enc::depends_set& deps );
    void set_ctrl_dependency( const hb_enc::depends_set& deps );
    void set_dependencies( const hb_enc::depends_set& data,
                           const hb_enc::depends_set& ctrl );

    friend std::ostream& operator<< ( std::ostream& stream,
                                      const symbolic_event& var ) {
      stream << var.name();
      return stream;
    }
    void debug_print(std::ostream& stream );
    z3::expr get_rd_expr(const tara::variable&);
    z3::expr get_wr_expr(const tara::variable&);
  };

  typedef std::unordered_map<std::string, se_ptr> name_to_ses_map;

  inline se_ptr
  mk_se_ptr_old( hb_enc::encoding& hb_enc, unsigned tid, unsigned instr_no,
                 const tara::variable& prog_v, std::string loc,
                 event_t et, se_set& prev_es) {
    std::string prefix = et == hb_enc::event_t::r ? "pi_" : "";
    tara::variable n_v = prefix + prog_v + "#" + loc;
    se_ptr e = std::make_shared<symbolic_event>( hb_enc.z3, tid, prev_es, instr_no,
                                                 n_v, prog_v, loc, et );
    e->guard = hb_enc.z3.mk_true();
    hb_enc.record_event( e );
    return e;
  }


  inline se_ptr
  mk_se_ptr_old( hb_enc::encoding& hb_enc, unsigned tid, unsigned instr_no,
                 std::string loc, event_t et, se_set& prev_es ) {
    se_ptr e =
      std::make_shared<symbolic_event>(hb_enc.z3, tid, prev_es, instr_no, loc, et);
    e->guard = hb_enc.z3.mk_true();
    hb_enc.record_event( e );
    return e;
  }

  //--------------------------------------------------------------------------
  // new calls
  // todo: streamline se all tstamps

  inline se_ptr
  mk_se_ptr( hb_enc::encoding& hb_enc, unsigned tid, se_set prev_es,
             z3::expr& path_cond, std::vector<z3::expr>& history_,
             const tara::variable& prog_v, std::string loc,
             event_t _et, hb_enc::o_tag_t ord_tag ) {
    tara::variable ssa_v = prog_v + "#" + loc;
    se_ptr e = std::make_shared<symbolic_event>( hb_enc.z3, tid, prev_es, 0,
                                                 ssa_v, prog_v, loc, _et);
    e->guard = path_cond;
    e->history = history_;
    e->o_tag = ord_tag;
    hb_enc.record_event( e );
    for(se_ptr ep  : prev_es) {
      ep->add_post_events( e, hb_enc.z3.mk_true() );
    }
    return e;
  }

  //todo: enable tagging for the fences of all kinds
  inline se_ptr
  mk_se_ptr( hb_enc::encoding& hb_enc, unsigned tid, se_set prev_es,
             z3::expr& path_cond, std::vector<z3::expr>& history_,
             std::string loc, event_t et,
             std::map<const hb_enc::se_ptr, z3::expr>& branch_conds,
             hb_enc::o_tag_t ord_tag = hb_enc::o_tag_t::na  ) {
    se_ptr e =
      std::make_shared<symbolic_event>( hb_enc.z3, tid, prev_es, 0, loc, et );
    e->guard = path_cond;
    e->history = history_;
    e->o_tag = ord_tag;
    hb_enc.record_event( e );
    // assert( z3::eq( branch_cond, hb_enc.z3.mk_true() ) ||
    //                 prev_es.size() == 1 );
    for( se_ptr ep  : prev_es ) {
      ep->add_post_events( e, branch_conds.at(ep) );
    }
    return e;
  }


  inline se_ptr
  mk_se_ptr( hb_enc::encoding& hb_enc, unsigned tid, se_set prev_es,
             z3::expr& path_cond, std::vector<z3::expr>& history_,
             std::string loc, event_t et,
             hb_enc::o_tag_t ord_tag = hb_enc::o_tag_t::na ) {
    std::map<const hb_enc::se_ptr, z3::expr> branch_conds;
    for( auto& ep : prev_es ) {
      branch_conds.insert( std::make_pair( ep, hb_enc.z3.mk_true() ) );
    }
    return mk_se_ptr( hb_enc, tid, prev_es, path_cond, history_, loc, et,
                      branch_conds, ord_tag );
  }

  //--------------------------------------------------------------------------

  struct se_hash {
    size_t operator () (const se_ptr &v) const {
      // return std::hash<std::string>()(v->e_v->name);
      return std::hash<std::string>()(v->name());
    }
  };

  struct se_equal :
    std::binary_function <symbolic_event,symbolic_event,bool> {
    bool operator() (const se_ptr& x, const se_ptr& y) const {
      return std::equal_to<std::string>()(x->name(), y->name());
    }
  };

  struct se_cmp :
    std::binary_function <symbolic_event,symbolic_event,bool> {
    bool operator() (const se_ptr& x, const se_ptr& y) const {
      return x->get_topological_order() < y->get_topological_order() ||
        ( x->get_topological_order() == y->get_topological_order() &&
          x < y
          );
    }
  };

  typedef std::set< se_ptr, se_cmp > se_tord_set;
  // typedef std::unordered_set<se_ptr, se_hash, se_equal> se_set;

  typedef std::unordered_map<tara::variable, se_ptr, tara::variable_hash, tara::variable_equal> var_to_se_map;

  typedef std::unordered_map<tara::variable, se_set, tara::variable_hash, tara::variable_equal> var_to_ses_map;

  typedef std::unordered_map<tara::variable, se_vec, tara::variable_hash, tara::variable_equal> var_to_se_vec_map;

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

  typedef std::unordered_map<se_ptr, depends_set, se_hash, se_equal> se_to_depends_map;
  typedef std::unordered_map< tara::variable,
                              depends_set,
                              tara::variable_hash,
                              tara::variable_equal> var_to_depends_map;


  depends pick_maximal_depends_set( hb_enc::depends_set& set );
  void join_depends_set( const se_ptr&, const z3::expr, depends_set& set );
  void join_depends_set( const depends& dep, depends_set& set );
  void join_depends_set( const depends_set& , depends_set& );
  void join_depends_set( const depends_set& , const depends_set&, depends_set&);
  void join_depends_set( const std::vector<depends_set>&, depends_set& );
  void join_depends_set( const std::vector<depends_set>&,
                         const std::vector<z3::expr>& conds,
                         hb_enc::depends_set& result );
  void meet_depends_set( const se_ptr&, const z3::expr, depends_set& set );
  void meet_depends_set( const depends& dep, depends_set& set );
  void meet_depends_set( const depends_set& , depends_set& );
  void meet_depends_set( const depends_set& , const depends_set&, depends_set&);
  void meet_depends_set( const std::vector<depends_set>&, depends_set& );
  void meet_depends_set( const std::vector<depends_set>&,
                         const std::vector<z3::expr>& conds,
                         hb_enc::depends_set& result );
  void pointwise_and( const depends_set&, z3::expr, depends_set& );

  //todo: deprecate this function
  inline bool is_po_old( const se_ptr& x, const se_ptr& y ) {
    if( x == y ) return true;
    if( x->tid != y->tid ) return false;
    if( x->get_instr_no() < y->get_instr_no() ) return true;
    if( x->get_instr_no() == y->get_instr_no() && x->is_rd() && y->is_wr() )
      return true;
    return false;
  }

  bool is_po_new( const se_ptr& x, const se_ptr& y );
  // must_before: if y occurs then, x must occur in the past
  // bool is_must_before( const se_ptr& x, const se_ptr& y );
  // must_after : if x occurs then, y must occur in the future
  // bool is_must_after ( const se_ptr& x, const se_ptr& y );

}

  void debug_print(std::ostream& out, const hb_enc::se_vec& set );
  void debug_print(std::ostream& out, const hb_enc::se_set& set );
  void debug_print(std::ostream& out, const hb_enc::se_to_ses_map& dep );
  void debug_print(std::ostream& out, const hb_enc::depends& dep );
  void debug_print(std::ostream& out, const hb_enc::depends_set& set);
}

#endif // TARA_CSSA_SYMBOLIC_EVENT_H
