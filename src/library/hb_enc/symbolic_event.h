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

  enum class event_kind_t {
      pre,  // initialization event
      r,    // read events
      w,    // write events
      barr, // barrier events
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
                    hb_enc::location_ptr loc, event_kind_t _et );

    symbolic_event( z3::context& ctx, hb_enc::encoding& _hb_enc,
                    unsigned _tid, unsigned instr_no,
                    hb_enc::location_ptr _loc, event_kind_t _et );

    // symbolic_event( hb_enc::encoding& _hb_enc, z3::context& ctx,
    //                 unsigned instr_no, unsigned thin_tid,
    //                 const cssa::variable& _v, const cssa::variable& _prog_v,
    //                 hb_enc::location_ptr _loc, event_kind_t _et);

  public:
    unsigned tid;
    cssa::variable v;               // variable with ssa name
    cssa::variable prog_v;          // variable name in the program
    hb_enc::location_ptr loc; // location in
    std::shared_ptr<tara::hb_enc::location> e_v; // variable for solver
    std::shared_ptr<tara::hb_enc::location> thin_v; // thin air variable
    event_kind_t et;
    se_set prev_events; // in straight line programs it will be singleton
                        // we need to remove access to  pointer
    z3::expr guard;
    inline std::string name() const {
      return e_v->name;
    }

    inline bool is_init() const {
      return et == event_kind_t::pre;
    }

    inline bool is_rd() const {
      return et == event_kind_t::r;
    }

    inline bool is_wr() const {
      return et == event_kind_t::w;
    }

    inline unsigned get_instr_no() const {
      return e_v->instr_no;
    }

    inline unsigned get_tid() const {
      return tid;
    }

    friend std::ostream& operator<< (std::ostream& stream,
                                     const symbolic_event& var) {
      stream << var.name();
      return stream;
    }
    void debug_print(std::ostream& stream );
    z3::expr get_var_expr(const cssa::variable&);
  };

  typedef std::unordered_map<std::string, se_ptr> name_to_ses_map;

  inline se_ptr mk_se_ptr( z3::context& _ctx, hb_enc::encoding& _hb_enc,
                           unsigned _tid, unsigned _instr_no,
                           const cssa::variable& _v, const cssa::variable& _prog_v,
                           hb_enc::location_ptr _loc, event_kind_t _et,
                           name_to_ses_map& se_store ) {
    se_ptr e = std::make_shared<symbolic_event>(_ctx, _hb_enc, _tid, _instr_no,
                                            _v, _prog_v, _loc, _et);
    se_store[e->name()] = e;
    return e;
  }

  inline se_ptr mk_se_ptr( z3::context& _ctx, hb_enc::encoding& _hb_enc,
                           unsigned _tid, unsigned _instr_no,
                           hb_enc::location_ptr _loc, event_kind_t _et,
                           name_to_ses_map& se_store ) {
    se_ptr e = std::make_shared<symbolic_event>(_ctx, _hb_enc, _tid, _instr_no,
                                            _loc, _et);
    se_store[e->name()] = e;
    return e;
  }

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


  inline bool is_po(const se_ptr& x, const se_ptr& y);
  void debug_print_se_set(const se_set& set, std::ostream& out);

}}

#endif // TARA_CSSA_SYMBOLIC_EVENT_H
