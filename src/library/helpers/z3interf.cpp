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


#include "z3interf.h"
#include <boost/algorithm/string.hpp>

using namespace z3;
using namespace std;
using namespace tara;
using namespace tara::helpers;

z3interf::z3interf(context& ctx) : c(ctx)
{
  Z3_update_param_value(ctx, "unsat_core", "true");
}

z3interf::~z3interf()
{
  
}


solver z3interf::create_solver() const
{
  return move(create_solver(c));
}

solver z3interf::create_solver(context& ctx)
{
  //params p(ctx);
  //p.set("unsat_core", true);
  //solver s(ctx, "QF_IRA");
  solver s(ctx);
  //s.set(p);
  // switch incremental mode on
  s.push();
  s.pop();
  return move(s);
}


///////////////////////////////////////////////
// Printing
///////////////////////////////////////////////

string z3interf::opSymbol(Z3_decl_kind decl) {
#ifdef HB_UNICODE
  switch(decl) {
    case Z3_OP_TRUE: return "⊤";
    case Z3_OP_FALSE: return "⊥";
    case Z3_OP_AND: return "∧";
    case Z3_OP_OR: return "∨";
    case Z3_OP_NOT: return "¬";
    default : return "unknownO";
  }
#else
switch(decl) {
  case Z3_OP_TRUE: return "True";
  case Z3_OP_FALSE: return "False";
  case Z3_OP_AND: return "/\\";
  case Z3_OP_OR: return "\\/";
  case Z3_OP_NOT: return "!";
  default : return "unknownO";
}

#endif
}

void z3interf::_printFormula(const expr& ast, ostream& out)
{
  switch(ast.kind()) {
    case Z3_APP_AST:
    {
      func_decl d = ast.decl();
      Z3_decl_kind dk = d.decl_kind();
      switch(dk) {
        case Z3_OP_UNINTERPRETED : {
          symbol n = d.name();
          out << n;
          }
          break;
        default: {
          string&& op = opSymbol(dk);
          unsigned args = ast.num_args();
          if (args==0) 
            out << op;
          else if (args==1) {
            out << op;
            expr arg = ast.arg(0);
            printFormula(arg, out);
          } else {
            for (unsigned i = 0; i<args; i++) {
              if (i>0) out << " " << op << " ";
              expr arg = ast.arg(i);
              printFormula(arg, out);
            }
          }}
          break;
      }
      break;
    }
    default:
      out << "unknownF";
  }
}

void z3interf::printFormula(const expr& expr, ostream& out)
{
  z3::expr simp = expr.simplify();
  _printFormula(simp, out);
}

///////////////////////////////////////
// Parsing
///////////////////////////////////////

inline std::string sanitise_string(std::string str) {
  boost::trim(str);
  while (str[0]=='(' && str[str.length()-1]==')') {
    str[0] = ' ';
    str[str.length()-1] = ' ';
    boost::trim(str);
  }
  return str;
}

expr z3interf::parseFormula(string str, const vector< string >& names, const vector< expr >& declarations) 
{
  assert (names.size() == declarations.size());
  unsigned s = declarations.size();
  Z3_symbol* symbols = new Z3_symbol[s];
  Z3_func_decl* decls = new Z3_func_decl[s];
  unsigned i = 0;
  for (unsigned i=0; i<s; i++) {
    symbols[i] = Z3_mk_string_symbol(c,names[i].c_str());
    decls[i] = declarations[i].decl();
    Z3_inc_ref(c, reinterpret_cast<Z3_ast>(decls[i]));
  }
  
  str = sanitise_string(str);
  
  string cmd = str.find_first_of(' ')!=string::npos ? "(assert (" + str + "))" : "(assert " + str + ")";
  expr ast(c);
  try {
	Z3_ast e = Z3_parse_smtlib2_string(c, cmd.c_str(), 0, NULL, NULL, s, symbols, decls);
	delete[] symbols;
	delete[] decls;
    ast = to_expr(c, e);
  } 
  catch (z3::exception e) {
    cerr << "Error parsing line \"" << str << "\"." << endl;
    throw;
  }
  
  // adjust reference counter for variable
  for (unsigned j=0; j<i; j++) {
    Z3_dec_ref(c, (Z3_ast)decls[j]);
  }
  
  return ast;
}

expr z3interf::parseFormula(string str, const input::variable_set& vars) 
{
  vector< string > names;
  vector< expr > declarations;
  for (input::variable v : vars) {
    string name = v;
    names.push_back(name);
    declarations.push_back(c.constant(name.c_str(), get_variable_sort(v)));
    name = name + ".";
    names.push_back(name);
    declarations.push_back(c.constant(name.c_str(), get_variable_sort(v)));
  }
  
  return parseFormula(str, names, declarations);
}

cssa::variable_set z3interf::get_variables(const expr& expr)
{
  cssa::variable_set result;
  if ((Z3_ast)expr != 0) {
    get_variables(expr, result);
  }
  return move(result);
}


void z3interf::get_variables(const expr& expr, cssa::variable_set& result)
{
  switch(expr.kind()) {
    case Z3_APP_AST:
      {
      func_decl d = expr.decl();
      Z3_decl_kind dk = d.decl_kind();
      switch(dk) {
        case Z3_OP_UNINTERPRETED : 
          {
            result.insert(cssa::variable(d.name().str(), expr.get_sort()));
          }
          break;
        default: 
          {
          unsigned args = expr.num_args();
          for (unsigned i = 0; i<args; i++) {
            z3::expr arg = expr.arg(i);
            get_variables(arg, result);
          }
          }
          break;
      }
      }
      break;
    case Z3_NUMERAL_AST:
      break;
    default: 
      throw "unsupported";
      break;
  }
}

void z3interf::decompose(expr conj, Z3_decl_kind kind, vector< expr >& result)
{
  switch(conj.kind()) {
    case Z3_APP_AST:
    {
      func_decl d = conj.decl();
      Z3_decl_kind dk = d.decl_kind();
      if (dk==kind) {
        unsigned args = conj.num_args();
        for (unsigned i = 0; i<args; i++) {
          expr arg = conj.arg(i);
          decompose(arg, kind, result);
        }
      } else {
        result.push_back(conj);
      }
      break;
    }
    default:
      result.push_back(conj);
  }
}

vector< expr > z3interf::decompose(expr conj, Z3_decl_kind kind)
{
  vector< expr > result;
  decompose(conj, kind, result);
  return result;
}

z3::sort z3interf::get_variable_sort(const input::variable& var) {
  switch (var.type) {
    case input::data_type::bv32 : return c.bv_sort(32);
    case input::data_type::bv16 : return c.bv_sort(16);
    case input::data_type::bv64 : return c.bv_sort(64);
    case input::data_type::integer : return c.int_sort();
    case input::data_type::boolean : return c.bool_sort();
    case input::data_type::real : return c.real_sort();
    case input::data_type::smt : {
      // translate the type
      string smt = sanitise_string(var.smt_type);
      smt = smt.find_first_of(' ')!=string::npos ? "(" + smt + ")" : smt;
      string cmd = "(declare-fun x () " + smt + ") (assert (= x x))";
      z3::expr ast = to_expr(c, Z3_parse_smtlib2_string(c, cmd.c_str(), 0, NULL, NULL, 0, NULL, NULL));
      return ast.arg(0).get_sort();
    }
  }
  throw "unsupported";
}


cssa::variable_set z3interf::translate_variables(input::variable_set vars) {
  cssa::variable_set newvars;
  for (input::variable v: vars) {
    cssa::variable newv(v.name, get_variable_sort(v));
    newvars.insert(newv);
  }
  return newvars;
}

//----------------
//support for gdb
void debug_print(const z3::expr& e) {
  Z3_ast ptr = e;
  if( ptr )
    std::cerr << e;
  else
    std::cerr << "uninitialized z3 expression!!";
  std::cerr << "\n";
}

void debug_print(const z3::model& m) {
  // Z3_ast ptr = m;
  // if( ptr )
    std::cerr << m;
  // else
  //   std::cerr << "uninitialized z3 model!!";
  std::cerr << "\n";
}

//----------------
