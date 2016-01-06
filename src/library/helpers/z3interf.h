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
#include <hb_enc/integer.h>

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
  
  hb_enc::integer _hb_encoding = hb_enc::integer(c);
  
public:
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
  
  z3::sort get_variable_sort(const input::variable& var);
  
  inline hb_enc::encoding& hb_encoding() { return _hb_encoding; }
  
  template <class value>
  static void min_unsat(z3::solver& sol, std::list<value>& items, std::function <z3::expr(value)> translate);
  template <class value>
  static void remove_implied(const z3::expr& fixed, std::list< value >& items, std::function< z3::expr(value) > translate);
    
};
  //----------------
  //support for gdb
  void print(const z3::expr&);
  //----------------

}}

#endif // Z3INTERF_H
