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
  std::vector<std::string> trace_strings =
    { "phase_selection" , "relevancy" };

  for( auto trace_string : trace_strings ) {
    Z3_enable_trace( trace_string.c_str() );
  }
  // Z3_enable_trace( "rewriter");
  // Z3_enable_trace( "rewriter_step");
  // Z3_enable_trace( "propagate_atoms" );
  // Z3_enable_trace( "relevancy" );
  // Z3_enable_trace( "special_relations");
}

z3interf::~z3interf() {}

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


Z3_decl_kind z3interf::get_decl_kind( z3::expr e ) {
  func_decl d = e.decl();
  return d.decl_kind();
}

bool z3interf::is_op(const expr& e, const Z3_decl_kind dk_op) {
  if( e.kind() ==  Z3_APP_AST ) {
      z3::func_decl d = e.decl();
      // Z3_decl_kind dk = d.decl_kind();
      if( d.decl_kind() == dk_op ) {
        return true;
      }
  }
  return false;
}

bool z3interf::is_implies(const expr& e) {return is_op(e,Z3_OP_IMPLIES); }
bool z3interf::is_neg    (const expr& e) { return is_op( e, Z3_OP_NOT ); }
bool z3interf::is_and    (const expr& e) { return is_op( e, Z3_OP_AND ); }
bool z3interf::is_or     (const expr& e) { return is_op( e, Z3_OP_OR  ); }

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

z3::expr z3interf::parseFormula(string str, const vector< string >& names, const vector< z3::expr >& declarations) 
{
  assert (names.size() == declarations.size());
  unsigned s = declarations.size();
  Z3_symbol* symbols = new Z3_symbol[s];
  Z3_func_decl* decls = new Z3_func_decl[s];
  unsigned i = 0;
  for (unsigned i=0; i<s; i++) {
    symbols[i] = Z3_mk_string_symbol(c,names[i].c_str());
    decls[i] = declarations[i].decl();
    // todo: the folloing inc causes memory leak. If enabled, the
    // corresponding dec_ref causes segmentation fault. !!!
    // Z3_inc_ref(c, reinterpret_cast<Z3_ast>(decls[i]));
  }
  
  str = sanitise_string(str);
  
  string cmd = str.find_first_of(' ')!=string::npos ? "(assert (" + str + "))" : "(assert " + str + ")";
  z3::expr ast(c);
  try {
	Z3_ast_vector es_ast =
          Z3_parse_smtlib2_string(c, cmd.c_str(), 0, NULL, NULL, s, symbols, decls);
	delete[] symbols;
	delete[] decls;
        expr_vector es = expr_vector( c, es_ast );
        if( es.size() != 1 ) {
          cerr << "Error non unique formula parsed!" <<  endl; throw;
        }
        ast = es[0];
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

tara::variable_set z3interf::get_variables(const expr& expr)
{
  tara::variable_set result;
  if ((Z3_ast)expr != 0) {
    get_variables(expr, result);
  }
  return move(result);
}


void z3interf::get_variables(const expr& expr, tara::variable_set& result)
{
  switch(expr.kind()) {
    case Z3_APP_AST:
      {
      func_decl d = expr.decl();
      Z3_decl_kind dk = d.decl_kind();
      switch(dk) {
        case Z3_OP_UNINTERPRETED : 
          {
            result.insert(tara::variable(d.name().str(), expr.get_sort()));
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

int z3interf::get_numeral_int(const expr& i) const {
  int val;
  if( Z3_get_numeral_int( c, i, &val) ) {
    return val;
  }else{
    z3interf_error( "too large int found!!" );
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


z3::expr z3interf::switch_sort( z3::expr& b, z3::sort& s ) {
  z3::sort bs = b.get_sort();
  if( bs.is_bool() && s.is_int() ) {
    if( is_false( b) ) {
      return c.int_val(0);
    }else if( is_true(b) ) {
      return c.int_val(1);
    }
  }else if( bs.is_int() && s.is_bool() ) {
    int v = get_numeral_int(b);
    if( v == 1 ) {
      return mk_true();
    }else if ( v  == 0 ) {
      return mk_false();
    }
  }
  z3interf_error( "failed to change sort!" );
}

bool z3interf::is_const( z3::expr& b ) {
  if( b.kind() == Z3_NUMERAL_AST ) return true;
  return false;
}


bool z3interf::is_bool_const( z3::expr& b ) {
  if( !b.is_bool() || b.kind() != Z3_APP_AST ) return false;
  return b.num_args() == 0;
}

std::string z3interf::get_top_func_name( z3::expr& b ) {
  assert( b.kind() == Z3_APP_AST );
  z3::func_decl d = b.decl();
  return d.name().str();
}

z3::expr z3interf::get_fresh_bool( std::string suff ) {
  static unsigned count = 0;
  stringstream b_name;
  b_name << "b_" << count << "_"<< suff;
  z3::expr b = c.bool_const(b_name.str().c_str());
  count++;
  return b;
}

z3::expr z3interf::get_fresh_int(  std::string suff ) {
  static unsigned count = 0;
  count++;
  std::string loc_name = "i_" + std::to_string(count) + "_" + suff;
  z3::expr loc_expr = c.int_const(loc_name.c_str());
  return loc_expr;
}



z3::sort z3interf::get_variable_sort(const input::variable& var) {
  switch (var.type) {
    case input::data_type::bv32    : return c.bv_sort(32);
    case input::data_type::bv16    : return c.bv_sort(16);
    case input::data_type::bv64    : return c.bv_sort(64);
    case input::data_type::integer : return c.int_sort();
    case input::data_type::boolean : return c.bool_sort();
    case input::data_type::real    : return c.real_sort();
    case input::data_type::smt : {
      // translate the type
      string smt = sanitise_string(var.smt_type);
      smt = smt.find_first_of(' ')!=string::npos ? "(" + smt + ")" : smt;
      string cmd = "(declare-fun x () " + smt + ") (assert (= x x))";
      z3::expr_vector es = c.parse_string( cmd.c_str() );
      assert( es.size() == 1 );
      z3::expr ast = es[0];
      // z3::expr ast = to_expr(c,Z3_parse_smtlib2_string(c, cmd.c_str(), 0, NULL, NULL, 0, NULL, NULL));
      return ast.arg(0).get_sort();
    }
  }
  throw "unsupported";
}

// todo: should be called is unsat
bool z3interf::is_unsat( z3::expr f ) const {
  auto s = create_solver();
  s.add( f );
  auto res = s.check();
  if( res == z3::check_result::unsat) return true;
  if( res == z3::check_result::sat) return false;
  z3interf_error("unsolved entailmet check");
  return false; // unreachable code: dummy return
}

bool z3interf::is_false( z3::expr f ) const {
  return z3::eq( f, mk_false() );
}

bool z3interf::is_true( z3::expr f ) const {
  return z3::eq( f, mk_true() );
}

bool z3interf::entails( z3::expr e1, z3::expr e2 ) const {
  // if e1 /\ !e2 is unsat, e1 => e2 is valid
  return is_unsat( e1 && !e2 );
}

bool z3interf::matched_sort( const z3::expr& l, const z3::expr& r ) {
  return z3::eq(l.get_sort(),r.get_sort());
}

tara::variable_set z3interf::translate_variables(input::variable_set vars) {
  tara::variable_set newvars;
  for (input::variable v: vars) {
    tara::variable newv(v.name, get_variable_sort(v));
    newvars.insert(newv);
  }
  return newvars;
}

bool z3interf::is_true_in_model( const z3::model& m, z3::expr e ) {
  z3::expr v = m.eval( e );
  Z3_lbool b = Z3_get_bool_value( (Z3_context)m.ctx(), (Z3_ast)v );
  return b == Z3_L_TRUE;
  // return is_true( v );
}

bool z3interf::is_false_in_model( const z3::model& m, z3::expr e ) {
  z3::expr v = m.eval( e );
  Z3_lbool b = Z3_get_bool_value( (Z3_context)m.ctx(), (Z3_ast)v );
  return b == Z3_L_FALSE;
  // return is_false( v );
}

z3::expr z3interf::simplify( z3::expr e ) {
  z3::goal g(c);
  g.add(e);

  z3::tactic simp(c, "simplify");
  z3::tactic ctx_simp(c, "ctx-simplify");
  z3::tactic final = simp & ctx_simp;

  z3::apply_result res = final.apply(g);

  g = res[0];
  e = c.bool_val(true);
  for (unsigned i = 0; i < g.size(); i++)
    e = e && g[i]; // first thing we put in
  return e;
}

//todo: make it a static function
z3::expr z3interf::simplify_or_vector( std::vector<z3::expr>& o_vec ) {
  unsigned sz = o_vec.size();

  for( unsigned i = 0; i < sz; i++ ) {
    z3::expr a = o_vec[i];
    z3::expr un_not_a = a;
    bool negated = false;
    while( Z3_OP_NOT == get_decl_kind( un_not_a ) ) {
      un_not_a = a.arg(0);
      negated = !negated;
    }
    z3::expr replace_term = negated ? mk_true() : mk_false();
    z3::expr_vector src(c); src.push_back( un_not_a );
    z3::expr_vector dst(c); dst.push_back( replace_term );
    for( unsigned j = 0; j < sz; j++ ) {
      if( i != j ) {
        o_vec[j] = o_vec[j].substitute( src, dst ); //todo : is it correct
        o_vec[j] = o_vec[j].simplify();
        if( is_true( o_vec[j] )  ) return o_vec[j];
      }
    }
  }
  z3::expr res = mk_false();
  for( z3::expr c : o_vec ) res = res || c;
  return res.simplify();
}

//----------------
//support for gdb
void tara::debug_print( std::ostream& out, const z3::expr& e ) {
  Z3_ast ptr = e;
  if( ptr )
    out << e;
  else
    out << "(uninitialized z3 expression!!)";
  // std::cerr << "\n";
}

void tara::debug_print(const z3::expr& e) {
  debug_print( std::cerr, e);
}

void tara::debug_print(const std::list<z3::expr>& es) {
  for(const z3::expr e : es ) {
    debug_print( e );
  }
}

void tara::debug_print(const std::vector< z3::expr >& es) {
  for(const z3::expr e : es ) {
    debug_print( e );
  }
}

void tara::debug_print( const z3::model& m ) {
  // Z3_ast ptr = m;
  // if( ptr )
    std::cerr << m;
  // else
  //   std::cerr << "uninitialized z3 model!!";
  std::cerr << "\n";
}

//----------------
