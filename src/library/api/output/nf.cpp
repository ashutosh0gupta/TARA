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
 *
 */

#include "nf.h"
#include "prune/remove_implied.h"
#include <boost/iterator/iterator_concepts.hpp>
#include "helpers/z3interf_templates.h"

using namespace tara;
using namespace tara::helpers;
using namespace tara::api::output;

using namespace std;

nf::nf(bool bad_dnf, bool bad_cnf, bool good_dnf, bool good_cnf, bool verify, bool no_opt) : output_base(), bad_dnf(bad_dnf), bad_cnf(bad_cnf), good_dnf(good_dnf), good_cnf(good_cnf), verify(verify), no_opt(no_opt)
{}

nf::nf() : output_base(), bad_dnf(true), bad_cnf(false), good_dnf(true), good_cnf(false), verify(false)
{}


void nf::prune_implied_within(nf::result_type& result, z3::solver& sol)
{
  for (auto& v:result) { 
    z3interf::min_unsat<hb_enc::hb>(sol, v, [](hb_enc::hb hb) { return hb; });
  }
}

void nf::prune_implied_outside(nf::result_type& result, z3::solver& sol)
{
  std::function <z3::expr(row_type)> translator = [&](row_type v) {
    z3::expr conj = sol.ctx().bool_val(true);
    for (hb_enc::hb& hb : v) {
      conj = conj && hb;
    }
    return !conj;
  };

  z3interf::min_unsat<row_type>(sol, result, translator);
}

void nf::prune_simple(nf::result_type& result, const z3::expr& po)
{
  /**
   * We are testing here: A ∧ B ∧ C. If A ∧ B ⇒ C then the formula reduces to A ∧ B
   */
  /*for (auto& v:result) { 
    z3interf::remove_implied<hb_enc::hb>(po, v, [](hb_enc::hb hb) { return hb; });
  }*/
  
  // do the same for rows
  /**
   * We are testing here: A ∨ B ∨ C. If C ⇒ A ∨ B then the formula reduces to A ∨ B
   */
  std::function <z3::expr(row_type)> translator = [&](row_type v) {
    z3::expr conj = po.ctx().bool_val(true);
    for (hb_enc::hb& hb : v) {
      conj = conj && hb;
    }
    return !conj;
  };
  
  z3interf::remove_implied<row_type>(po, result, translator);
}


void nf::print(ostream& stream, bool machine_readable, bool bad_dnf, bool bad_cnf, bool good_dnf, bool good_cnf, bool verify) const
{
  ready_or_throw();

  bool print_header = !machine_readable;
  if (bad_dnf) {
    if (print_header) stream << "Bad DNF" << endl;
    print_one(stream, machine_readable, get_result(true, true), true);
    if (verify) verify_result(stream, true, get_result_expr(true, true));
    if (print_header) stream << endl;
  }
  if (bad_cnf) {
    if (print_header) stream << "Bad CNF" << endl;
    print_one(stream, machine_readable, get_result(true, false), false);
    if (verify) verify_result(stream, true, get_result_expr(true, false));
    if (print_header) stream << endl;
  }
  
  if (good_dnf) {
    if (print_header) stream << "Good DNF" << endl;
    print_one(stream, machine_readable, get_result(false, true), true);
    if (verify) verify_result(stream, false, get_result_expr(false, true));
    if (print_header) stream << endl;
  }
  if (good_cnf) {
    if (print_header) stream << "Good CNF" << endl;
    print_one(stream, machine_readable, get_result(false, false), false);
    if (verify) verify_result(stream, false, get_result_expr(false, false));
    if (print_header) stream << endl;
  }
}

void nf::print(ostream& stream, bool machine_readable) const
{
  print(stream, machine_readable, bad_dnf, bad_cnf, good_dnf, good_cnf, verify);
}

void nf::print_one(ostream& stream, bool machine_readable, const nf::result_type& result, bool dnf_not_cnf)
{
  for(list<row_type>::const_iterator conj = result.begin(); conj != result.end(); conj++) {
    if (!machine_readable && conj->size()>1)  
      stream << "( ";
    for (auto hb = conj->begin(); hb != conj->end(); hb++) {
      auto hb1 = *hb;
      if (!machine_readable)
        stream << hb1 << " ";
      else
        stream << hb1.loc1 << "<" << hb1.loc2 << " ";
      if (!last_element(hb, *conj))
        if (!machine_readable)
          stream << (dnf_not_cnf ? z3interf::opSymbol(Z3_OP_AND) : z3interf::opSymbol(Z3_OP_OR)) << " ";
    }
    if (!machine_readable && conj->size()>1)
      stream << ") ";
    if (!last_element(conj, result))
      if (!machine_readable)
        stream << (dnf_not_cnf ? z3interf::opSymbol(Z3_OP_OR) : z3interf::opSymbol(Z3_OP_AND));
      stream << endl;
  }
}

z3::expr nf::get_result_expr(bool bad_not_good, bool dnf_not_cnf) const
{
  const result_type& result = get_result(bad_not_good, dnf_not_cnf);
  z3::expr res = _output->ctx().bool_val(!dnf_not_cnf);
  for(auto v : result) {
    z3::expr conj = _output->ctx().bool_val(dnf_not_cnf);
    for (auto hb : v) {
      if (dnf_not_cnf)
        conj = conj && hb;
      else 
        conj = conj || hb;
    }
    if (dnf_not_cnf)
      res = res || conj;
    else
      res = res && conj;
  }
  return res;
}

const nf::result_type& nf::get_result(bool bad_not_good, bool dnf_not_cnf) const
{
  result_type& result = bad_not_good ?
    (dnf_not_cnf ? result_bad_dnf : result_bad_cnf) :
    (dnf_not_cnf ? result_good_dnf : result_good_cnf);
    
  if (result.size() > 0) return result;
  
  //init the variable
  if (bad_not_good && dnf_not_cnf) {
    create_dnf(*_output, result, *_hb_encoding);
    if (!no_opt) {
      prune_implied_within(result, *sol_good);
      prune_implied_outside(result, *sol_bad);
      prune_simple(result, program->phi_po);
    }
  }
  else if (!bad_not_good && dnf_not_cnf) {
    create_dnf(!*_output, result, *_hb_encoding);
    if (!no_opt) {
      prune_implied_within(result, *sol_bad);
      prune_implied_outside(result, *sol_good);
    }
  }
  else if (bad_not_good && !dnf_not_cnf) {
    result = get_result(false, true);
    inverse_result(result);
  }
  else if (!bad_not_good && !dnf_not_cnf) {
    result = get_result(true, true);
    inverse_result(result);
  }
  
  return result;
  
}


void nf::create_dnf(const z3::expr& formula, nf::result_type& result, const hb_enc::encoding& hb_encoding)
{
  result.clear();
  z3::context& c = formula.ctx();
  assert (Z3_get_bool_value(c, formula) == Z3_L_UNDEF);
  
  z3::goal g(c);
  g.add(formula);
  z3::tactic s(c, "simplify");
  z3::params p(c);
  p.set("elim-and", true);
  z3::tactic s1(s, p);
  
  z3::tactic sc(c, "split-clause");
  z3::tactic sk(c, "skip");
  
  z3::tactic final = (s1 & (repeat(sc | sk)));
  z3::apply_result res = final.apply(g);
  // go through disjuncts
  for(unsigned i=0; i<res.size(); i++) {
    z3::goal conj = res[i];
    row_type _conj;
    for (unsigned j=0; j<conj.size(); j++) {
      auto hb = hb_encoding.get_hb(conj[j], true);
      assert (hb);
      _conj.push_back(*hb);
    }
    result.push_back(_conj);
  }
}

void nf::inverse_result(nf::result_type& result)
{
  for (auto& x : result)
    for (hb_enc::hb& hb : x) {
      hb = hb.negate();
    }
}

bool nf::verify_result(ostream& stream, bool bad_not_good, z3::expr result) const
{
  sol_bad->push();
  sol_good->push();
  
  stream << "Verify: ";
  // sanity check
  z3::check_result r1;
  bool first_wrong = false;
  bool second_wrong = false;
  string msg;
  if (!bad_not_good) {
    msg = "bad trace in result";
    sol_bad->add(result);
    r1 = sol_bad->check();
    if (r1==z3::check_result::sat) {
      first_wrong = true;
      stream << msg;
    }
  }
  
  z3::check_result r2;
  if (bad_not_good) {
    msg = "bad traces not in result";
    sol_bad->add(!result);
    r2 = sol_bad->check();
    if (r2==z3::check_result::sat) {
      if (first_wrong)
        stream << "; ";
      stream << msg;
      second_wrong= true;
    }
  }  
  if (!first_wrong && !second_wrong)
    stream << "Passed";
  
  stream << endl;
  
  sol_bad->pop();
  sol_good->pop();
  
  return !first_wrong && !second_wrong;
}

void nf::gather_statistics(api::metric& metric) const
{
  metric.sum_hbs_after = 0;
  metric.disjuncts_after = 0;
  for (auto& row : get_result(true, true)) {
    metric.disjuncts_after++;
    metric.sum_hbs_after += row.size();
  }
}
