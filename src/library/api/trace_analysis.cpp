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

#include "trace_analysis.h"
#include "helpers/z3interf.h"
#include "input/ctrc_input.h"
#include "cssa/wmm.h"
#include "api/options.h"
#include "api/output/nf.h"
#include "helpers/helpers.h"
#include "hb_enc/integer.h"
#include "prune/prune_base.h"
#include "prune/prune_functions.h"
#include "prune/unsat_core.h"
#include "prune/data_flow.h"
#include "prune/remove_implied.h"
#include "program/program.h"
#include "cinput/cinput.h"
#include <stdexcept>
#include <vector>
#include <chrono>
#include <fstream>

using namespace tara::api;
using namespace tara;
using namespace tara::helpers;

using namespace std;

trace_analysis::trace_analysis(options& options, helpers::z3interf& _z3) :
  _options(options), z3(_z3), hb_encoding(_z3) {}

// void trace_analysis::input(string input_file, mm_t mm) {
void trace_analysis::input(string input_file) {
  if( has_suffix( input_file, ".c" ) ||
      has_suffix( input_file, ".cpp" ) || has_suffix( input_file, ".cc" ) ) {

    program = unique_ptr<tara::program>(cinput::parse_cpp_file( z3, _options,
                                                                hb_encoding,
                                                                input_file ));
    if( _options.mm != mm_t::none ) {
      program->set_mm( _options.mm );
    }else{
      //todo: make sc default memory model -- the following error must not occur
      trace_error( "cinput and no memory model specified!!" );
    }
  }else if( has_suffix( input_file, ".ctrc" ) ) {
    program = unique_ptr<ctrc::program>(ctrc_input::parse_ctrc_file(z3,_options,
                                                                    hb_encoding,
                                                                    input_file ));
  }else{
    trace_error( "a file with unknown extensition passed!!" );
  }

  if( program->is_mm_declared() ) {
    cssa::wmm_event_cons mk_cons( z3, _options, hb_encoding,  *program );
    mk_cons.run();
  }

  if (_options.print_phi) {
    _options.out() << "(" << endl;
    _options.out() << "phi_vd"  << endl << program->phi_vd << endl;
    _options.out() << "phi_pre" << endl << program->phi_pre << endl;
    _options.out() << "phi_prp" << endl << program->phi_prp << endl;
    _options.out() << "phi_fea" << endl << program->phi_fea << endl;
    if( _options.mm == mm_t::none ) {
      _options.out() << "phi_po" << endl << program->phi_po << endl;
      _options.out() << "phi_pi" << endl << program->phi_pi << endl;
      _options.out() << endl;
    }
    _options.out() << ")" << endl;
  }
}

void trace_analysis::gather_statistics(metric& metric) {
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");
  program->gather_statistics( metric );
}

void trace_analysis::connect_read_writes( z3::solver& result ) {
  if( program->is_mm_declared() ) {
    result.add( program->phi_po );
    result.add( program->phi_ses);
  }else{
    result.add( program->phi_po );
    result.add( program->phi_pi );
  }
}

z3::solver trace_analysis::make_bad()
{
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");
  z3::solver result = z3.create_solver();

  result.add(program->phi_vd);
  connect_read_writes( result ); //wmm diversion//result.add(program->phi_pi);
  result.add(program->phi_pre);
  result.add(!program->phi_prp);

  return move(result);
}

z3::solver trace_analysis::make_good(bool include_infeasable)
{
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");
  z3::solver result = z3.create_solver();

  result.add(program->phi_vd);
  connect_read_writes( result );//wmm diversion//result.add(program->phi_pi);
  result.add(program->phi_pre);
  result.add(program->phi_prp);
  if( !program->is_mm_declared() ) //wmm disable TODO: hack??
  if (!include_infeasable)
    result.add(program->phi_fea);

  return move(result);
}

trace_result trace_analysis::seperate(output::output_base& output, tara::api::metric& metric)
{
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");


  options& o = _options;

  //z3::solver sol_good = make_good(false);
  z3::solver sol_bad = make_bad();
  // std::cout << sol_bad;assert(false);

  prune::prune_chain prune_chain;
  if (o.prune_chain.find(string("data_flow"))!=string::npos) metric.data_flow = true;
  if (o.prune_chain.find(string("unsat_core"))!=string::npos) metric.unsat_core = true;
  if (!prune::build_prune_chain(o.prune_chain, prune_chain, z3, *program, make_good(true))) // must be true (=include_infeasable) to be sound while pruning (because we use inputs) #(see below)
      throw "Invalid prune chain";

  // some check to ensure there are any good feasable traces at all
  // without this check we may get problems down the line
  // (because below we include infeasable in good and later in nf.cpp we do not, leaning to an empty set of
  if( 0 ) { // print good constraints
    std::cerr << make_good(false);
  }
  if (make_good(false).check() == z3::check_result::unsat) {
    return trace_result::always;
  }

  z3::expr result = z3.c.bool_val(false);
  std::vector< hb_enc::hb_vec > hb_result;
  z3::check_result r;
  while ((r=sol_bad.check()) == z3::check_result::sat) {
    auto start_time = chrono::steady_clock::now();
    z3::model m = sol_bad.get_model();
    o.round++;

    // printing
    if (o.print_rounds >= 1) {
        o.out() << "Round " << o.round << endl;
      if( program->is_original() ) {
        o.out() << "Example found:" << endl;
        ((ctrc::program*)(program.get()))->print_hb(m, o.out(), o.machine );
      }else{
        program->print_execution( "round-"+std::to_string(o.round), m );
      }
      if( o.print_rounds >=2 ) o.out() << "Model:" << endl << m << endl << endl;
      o.out() << endl << endl;
    }

    // read solver for hbs constraints
    hb_enc::hb_vec hbs = hb_encoding.get_hbs(m);
    // remove unnecessary hbs
    prune::apply_prune_chain( prune_chain, o, m, hbs );

    z3::expr forbid = hb_encoding.mk_expr( hbs );

    //printing
    if (o.machine && o.print_pruning >=1 ) {
      Z3_lbool forbid_value = Z3_get_bool_value(forbid.ctx(),forbid.simplify());
      if (forbid_value == Z3_L_UNDEF) {
        output::nf::result_type disj;
        output::nf::create_dnf(forbid, disj, hb_encoding);
        output::nf::print_one(cout, true, disj, true);
      }
    }
    if( o.print_rounds >= 1) {
      o.out() << "Round result:" << endl;
      tara::debug_print( o.out(), hbs );
      o.out() << endl;
    }

    sol_bad.add(!forbid);
    result = result || forbid;
    hb_result.push_back( hbs );

    //stats
    auto end_time = chrono::steady_clock::now();
    metric.sum_round_time += chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
    metric.sum_hbs += hbs.size();
    metric.disjuncts++;
    metric.rounds++;
  }
  if (r == z3::check_result::unknown) {
    cerr << "Solver undecided: " << sol_bad.reason_unknown() << endl;
    return trace_result::undecided;
  }

  output.init(hb_encoding, make_bad(), make_good(false), program);

  if (o.print_output >= 3) {
    o.out() << "Result as formula:" << endl;
    o.out() << result << endl;
  }

  Z3_lbool b = Z3_get_bool_value(result.ctx(), result.simplify());

  trace_result return_result;
  switch (b) {
    case Z3_L_TRUE:
      return_result = trace_result::always;
      break;
    case Z3_L_FALSE:
      return_result = trace_result::never;
      break;
    default: {

      // handling the case of rf exprs which contain event conds
      // removing the guards before simplification step
      // todo: make the following step transparent
      result =  z3.mk_false();
      for( auto& hbs : hb_result ) {
        z3::expr forbid = z3.mk_true();
        for( auto& hb : hbs ) {
          z3::expr hb_expr = *hb;
          if( hb->is_rf() && z3.is_implies( hb_expr ) ) {
            hb_expr = hb_expr.arg(1);
          }
          forbid = forbid && hb_expr;
        }
        result = result || forbid;
      }

      // check if all traces are bad
      /*sol_good.add(!result);
      if (sol_good.check() == z3::unsat)
        return trace_result::always;*/

      // simplify to summarise solutions
      // z3::goal g(z3.c);
      // g.add(result);
      // z3::tactic simp(z3.c, "simplify");

      // z3::tactic ctx_simp(z3.c, "ctx-simplify");

      // z3::tactic final = simp & ctx_simp;

      // z3::apply_result res = final.apply(g);

      // g = res[0];
      // result = z3.c.bool_val(true);
      // for (unsigned i = 0; i < g.size(); i++)
      //   result = result && g[i]; // first thing we put in

      // if (o.print_output >= 1) {
      //   o.out() << "Simplified formula:" << endl;
      //   o.out() << result << endl;
      // }

      if( o.mm != mm_t::c11 ) {
        result = z3.simplify( result );
      }

      output.output(result);

      return_result = trace_result::depends;
      break;
    }
  }

  return return_result;
}

void trace_analysis::dump_smt2() {

  boost::filesystem::path gname = _options.output_dir;
  gname += "good.smt2";
  std::cerr << "dumping smt2 file : " << gname << std::endl;

  std::ofstream myfile;
  myfile.open( gname.c_str() );
  if( myfile.is_open() ) {
    myfile << make_good(false);
    myfile << "(check-sat)\n";
  }else{
    trace_error( "failed to open " << gname.c_str() );
  }
  myfile.close();

  boost::filesystem::path bname = _options.output_dir;
  bname += "bad.smt2";
  std::cerr << "dumping smt2 file : " << bname << std::endl;

  std::ofstream b_myfile;
  b_myfile.open( bname.c_str() );
  if( b_myfile.is_open() ) {
    b_myfile << make_bad();
    b_myfile << "(check-sat)\n";
  }else{
    trace_error( "failed to open " << bname.c_str() );
  }
  b_myfile.close();
}

void trace_analysis::test_bad() {
  options& o = _options;
  z3::check_result r;
  auto sol_bad = make_bad();

  if((r=sol_bad.check()) == z3::check_result::sat) {
    z3::model m = sol_bad.get_model();
    if( o.print_rounds >=2 ) o.out() << "Model:" << endl << m << endl << endl;
    // printing
    if( program->is_original() ) {
      o.out() << "Example found:" << endl;
      ((ctrc::program*)(program.get()))->print_hb(m, o.out(), o.machine );
    }else{
      program->print_execution( "bad-test", m );
    }
  }else{
    o.out() << "No bad execution found!" << endl;
  }
}

void trace_analysis::test_good(bool include_infeasable) {
  options& o = _options;
  z3::check_result r;
  auto sol_good = make_good(include_infeasable);

  if((r=sol_good.check()) == z3::check_result::sat) {
    z3::model m = sol_good.get_model();
    if( o.print_rounds >=2 ) o.out() << "Model:" << endl << m << endl << endl;
    // printing
    if( program->is_original() ) {
      o.out() << "Example found:" << endl;
      ((ctrc::program*)(program.get()))->print_hb(m, o.out(), o.machine );
    }else{
      program->print_execution( "good-test", m );
    }
  }else{
    o.out() << "No good execution found!" << endl;
  }
}

bool trace_analysis::atomic_sections(vector< hb_enc::as >& output, bool merge_as)
{
  options& o = _options;

  z3::solver sol_bad = make_bad();

  // list of all atomic sections
  unordered_map<Z3_ast, hb_enc::as> trigger_map;
  z3::expr_vector assumptions(z3.c);
  for (unsigned t=0; t<program->size(); t++) {
    const tara::thread& th = (*program)[t];
    for (unsigned i=0; i<th.size()-1; i++) {
      hb_enc::as a = hb_encoding.make_as(th[i].loc, th[i+1].loc);
      z3::expr trigger = z3.c.fresh_constant("as_assumption", z3.c.bool_sort());
      sol_bad.add(implies(trigger, a));
      assumptions.push_back(trigger);
      trigger_map.insert(std::make_pair(trigger, a));
    }
  }

  if (sol_bad.check(assumptions) != z3::unsat)
    return false; // we cannot find an atomic section
  z3::expr_vector core = sol_bad.unsat_core();

  if (o.print_output >= 1)
    o.out() << "Original Atomic sections" << endl;
  unsigned total_instr = 0;
  for(unsigned i=0; i<core.size(); i++) {
    auto t = trigger_map.find(core[i]);
    if (t != trigger_map.end()) {
      hb_enc::as as = t->second;
      if (o.print_output >= 1)
        o.out() << as << endl;
      // check if this can be combined with an existing atomic section
      if (merge_as) {
        for (auto j = output.begin(); j!=output.end(); ) {
          if (j->loc1 == as.loc2) {
            as = hb_encoding.make_as(as.loc1, j->loc2);
            total_instr -= j->loc2->instr_no - j->loc1->instr_no;
            j = output.erase(j);
          } else if (j->loc2 == as.loc1) {
            as = hb_encoding.make_as(j->loc1, as.loc2);
            total_instr -= j->loc2->instr_no - j->loc1->instr_no;
            j = output.erase(j);
          } else {
            j++;
          }
        }
      }
      total_instr += as.loc2->instr_no - as.loc1->instr_no;
      output.push_back(as);
    }
  }
  assert (total_instr == core.size());


  return true;
}

bool trace_analysis::check_ambigious_traces(unordered_set< string >& result)
{
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");

  // rename the variables to make the different
  auto po_vars = z3.get_variables(program->phi_po);
  auto vars = z3.get_variables(program->phi_vd && program->phi_pi && program->phi_pre && program->phi_prp);

  z3::expr_vector o(z3.c);
  z3::expr_vector n(z3.c);

  for (auto v: vars) {
    if (po_vars.find(v)==po_vars.end()) {
      o.push_back(z3.c.constant(v.name.c_str(), v.sort));
      n.push_back(z3.c.constant((v.name+"_g").c_str(), v.sort));
    }
  }

  z3::solver sol1 = z3.create_solver();
  sol1.add(program->phi_vd && program->phi_pi && program->phi_pre && program->phi_prp);
  sol1.add(program->phi_po);
  z3::expr good = program->phi_vd && program->phi_pi && program->phi_pre && !program->phi_prp;
  sol1.add(good.substitute(o, n));

  z3::check_result r = sol1.check();
  assert(r!=z3::unknown);
  if (r==z3::sat) {
    z3::model m = sol1.get_model();
    for (auto g : program->globals) {
      tara::variable g1 = g + "#pre";
      tara::variable g2 = g1 + "_g";
      z3::expr e = m.eval((z3::expr)g1 != g2);
      Z3_lbool b = Z3_get_bool_value(e.ctx(), e);
      if (b == Z3_L_TRUE)
        result.insert(g);
    }

    //assert(result.size() > 0);
    // there can be other sources of non-determinism
    return true;
  }

  return false;
}

/**
 * # (footnote, this program will go wrong if infeasable is not included in ctp-bar.
 * global: x y z
 *
 * pre: (= x 0)
 *
 * thread t1:
 * a1: (= x. 1)
 * a2: (= y. 1)
 *
 * thread t2:
 * b1: assume(= x 1)
 * b2: assert(= y 1)
 * b3: assume(= z 1)
 *
 * (Reason: with the inputs we may fix z to 10 and therefore and then remove all HBs)
 */
