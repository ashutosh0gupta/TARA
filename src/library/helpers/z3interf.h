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

#ifndef Z3INTERF_H
#define Z3INTERF_H

#include <z3++.h>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include "helpers/helpers.h"
// #include <hb_enc/integer.h>

#include <boost/regex.hpp>

namespace tara {
namespace helpers {


  
class z3interf
{
public:
  z3interf(z3::context& ctx);
  ~z3interf();
  z3::context& c;
private:
  z3interf(z3interf const & s);
  z3interf & operator=(z3interf const & s);

  //void add_hb(z3::expr a, std::string a_name, z3::expr b, std::string b_name);
    
  void _printFormula(const z3::expr& ast, std::ostream& out);
    
  static void get_variables(const z3::expr& expr, cssa::variable_set& result);
  
  // hb_enc::integer _hb_encoding = hb_enc::integer(c);
  
public:

  struct expr_hash {
    size_t operator () (const z3::expr& a) const {
      Z3_ast ap = a;
      size_t hash = std::hash<Z3_ast>()(ap);
      return hash;
    }
  };

  struct expr_equal :
    std::binary_function <z3::expr,z3::expr,bool> {
    bool operator() (const z3::expr& x, const z3::expr& y) const {
      return z3::eq( x, y );
      // return std::equal_to<std::z3::expr>()(x->e_v->name, y->e_v->name);
    }
  };

  // bool isEq( z3::expr& a, z3::expr&b ) {
  //   Z3_ast ap = a;
  //   Z3_ast bp = b;
  //   return ap == bp;
  // }

  // struct exprComp {
  //   bool operator() (const z3::expr& a, const z3::expr& b) const
  //   {
  //     Z3_ast ap = a;
  //     Z3_ast bp = b;
  //     return ap < bp;
  //   }
  // };

  static std::string opSymbol(Z3_decl_kind decl);

  z3::solver create_solver() const;
  static z3::solver create_solver(z3::context& ctx);
  
  void printFormula(const z3::expr& expr, std::ostream& out);
  z3::expr parseFormula(std::string str, const input::variable_set& vars);
  z3::expr parseFormula(std::string str, const std::vector<std::string>& names, const std::vector<z3::expr>& declarations);
  cssa::variable_set translate_variables(input::variable_set vars);
    
  // getting the information which happens-before relations are important
  static cssa::variable_set get_variables(const z3::expr& expr);
  
  static std::vector<z3::expr> decompose(z3::expr conj, Z3_decl_kind kind);
  static void decompose(z3::expr conj, Z3_decl_kind kind, std::vector< z3::expr >& result);
  

  z3::sort mk_sort(const char* s) const { return c.uninterpreted_sort(s); }

  z3::expr mk_true() const { return c.bool_val(true ); }
  z3::expr mk_false() const { return c.bool_val(false); }
  z3::expr mk_emptyexpr(){ return z3::expr(c); }

  z3::sort get_variable_sort(const input::variable& var);

  z3::expr get_fresh_bool( std::string suff = "" );
  z3::expr get_fresh_int(  std::string suff = "" );

  bool is_false( z3::expr ) const;
  bool is_true( z3::expr ) const;
  bool is_sat( z3::expr ) const;
  bool entails( z3::expr e1, z3::expr e2 ) const;

  bool is_bool_const( z3::expr );
  std::string get_top_func_name( z3::expr b );

  template <class value>
  static void min_unsat(z3::solver& sol, std::list<value>& items, std::function <z3::expr(value)> translate, bool potentially_input_sat = false);
  template <class value>
  static void remove_implied(const z3::expr& fixed, std::list< value >& items, std::function< z3::expr(value) > translate);
  z3::expr simplify_or_vector( std::vector<z3::expr>& o_vec );
  Z3_decl_kind get_decl_kind( z3::expr e );
};
  // we need this xor, since the default xor in c++ interface is for bvxor
  inline z3::expr _xor( z3::expr const & a, z3::expr const & b ) {
    check_context(a, b);
    Z3_ast r = Z3_mk_xor(a.ctx(), a, b);
    return z3::expr(a.ctx(), r);
  }


}
  //----------------
  //support for gdb
  void debug_print( std::ostream& out, const z3::expr& );
  void debug_print( const z3::expr& );
  void debug_print( const std::list<z3::expr>& );
  void debug_print( const z3::model& );
  void debug_print( const std::vector< z3::expr >& es );
  void debug_print( const z3::model& m );
  //----------------
}

#endif // Z3INTERF_H
