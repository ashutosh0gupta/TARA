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
#include "input/parse.h"
#include "cssa/program.h"
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

using namespace tara::api;
using namespace tara;
using namespace tara::helpers;

using namespace std;

trace_analysis::trace_analysis(options options, z3::context& ctx) : _options(options), z3(ctx), hb_encoding(ctx)
{
}

void trace_analysis::input(string input_file)
{
  input(input_file,mm_t::none);
}

void trace_analysis::input(string input_file, mm_t mm)
{
  if( has_suffix(input_file, ".c" ) || has_suffix(input_file, ".cpp" ) ) {
     program = unique_ptr<tara::program>(cinput::parse_cpp_file( z3, _options,
                                                                 hb_encoding,
                                                                 input_file ));
    if( mm != mm_t::none ) {
      program->set_mm( mm );
    }
    if( program->is_mm_declared() ) {
      cssa::wmm_event_cons mk_cons( z3, _options, hb_encoding,  *program );
      mk_cons.run();
      if( _options.print_phi && program->get_mm() != mm_t::none ) {
        _options.out() << "(" << endl
                       << "phi_pre : \n" << program->phi_pre << endl
                       <<"fea      : \n" << program->phi_fea << "\n"
                       <<"vd       : \n" << program->phi_vd  << "\n"
                       <<"prp      : \n" << program->phi_prp << "\n"
                       << ")" << endl;
      }
    }else{ trace_error( "cinput and no memory model specified!!" ); }
  }else{
    input::program pa = input::parse::parseFile(input_file.c_str());
    //--------------------------------------------------------------------------
    // start of wmm support
    //--------------------------------------------------------------------------
    if( mm != mm_t::none ) {
      pa.set_mm( mm );
    }
    //--------------------------------------------------------------------------
    // end of wmm support
    //--------------------------------------------------------------------------
    input(pa);
  }
}

void trace_analysis::input(input::program& input_program)
{
  input_program.convert_instructions(z3, hb_encoding);
  input_program.check_correctness();
  program = unique_ptr<cssa::program>(new cssa::program( z3,
                                                         _options,
                                                         hb_encoding,
                                                         input_program));

  if( program->is_mm_declared() ) {
    cssa::wmm_event_cons mk_cons( z3, _options, hb_encoding,  *program);
    mk_cons.run();
  }

  if (_options.print_phi) {
  //--------------------------------------------------------------------------
  //start of wmm support
  //--------------------------------------------------------------------------
    if( input_program.get_mm() != mm_t::none ) {
      _options.out() << "(" << endl;
      _options.out() << "phi_pre : " << endl << program->phi_pre << endl;
      _options.out() <<"fea : \n"<< program->phi_fea<<"\n";
      _options.out() <<"vd : \n" << program->phi_vd<<"\n";
      _options.out() <<"prp : \n"<< program->phi_prp<<"\n";
      _options.out() << ")" << endl;
    }else{
  //--------------------------------------------------------------------------
  //end of wmm support
  //--------------------------------------------------------------------------
      _options.out() << "phi_po" << endl << program->phi_po << endl;
      _options.out() << "phi_vd" << endl << program->phi_vd << endl;
      _options.out() << "phi_pi" << endl << program->phi_pi << endl;
      _options.out() << "phi_pre" << endl << program->phi_pre << endl;
      _options.out() << "phi_prp" << endl <<program->phi_prp << endl;
      _options.out() << "phi_fea" << endl <<program->phi_fea << endl;
      _options.out() << endl;
    }
  }

}

void trace_analysis::gather_statistics(metric& metric)
{
  if (program==nullptr)
    throw logic_error("Input needs to be initialised first.");
  if( program->is_original() ) {
    auto p = (cssa::program*)(program.get());
    p->gather_statistics( metric );
  }else{
    program->gather_statistics( metric );
    //trace_error( "no stats for new programs!!" );
  }

}

//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
void trace_analysis::connect_read_writes( z3::solver& result )
{
  if( program->is_mm_declared() ) {
    result.add( program->phi_po );
    result.add( program->phi_ses);
  }else{
    result.add( program->phi_po );
    result.add( program->phi_pi );
  }
}
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------

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

  prune::prune_chain prune_chain;
  if (o.prune_chain.find(string("data_flow"))!=string::npos) metric.data_flow = true;
  if (o.prune_chain.find(string("unsat_core"))!=string::npos) metric.unsat_core = true;
  if (!prune::build_prune_chain(o.prune_chain, prune_chain, z3, *program, make_good(true))) // must be true (=include_infeasable) to be sound while pruning (because we use inputs) #(see below)
      throw "Invalid prune chain";
  
  // some check to ensure there are any good feasable traces at all
  // without this check we may get problems down the line
  // (because below we include infeasable in good and later in nf.cpp we do not, leaning to an empty set of 

  if (make_good(false).check() == z3::check_result::unsat) {
    return trace_result::always;
  }
  
  z3::expr result = z3.c.bool_val(false);
  z3::check_result r;
  while ((r=sol_bad.check()) == z3::check_result::sat) {
    auto start_time = chrono::steady_clock::now();
    z3::model m = sol_bad.get_model();
    o.round++;
    if (o.print_rounds >= 1) {
        o.out() << "Round " << o.round << endl;
    }
    if (o.print_rounds >= 1) {
      if( program->is_original() ) {
        auto p = (cssa::program*)(program.get());
        o.out() << "Example found:" << endl;
        if (o.machine)
          p->print_hb(m, cout, true);
        else
          p->print_hb(m, o.out());
      }
    }

    // list<z3::expr> hbs = hb_encoding.get_hbs(m);
    // z3::expr forbid = prune::apply_prune_chain(prune_chain, hbs, m, o.print_pruning, o.out(), hb_encoding);

    hb_enc::hb_vec hbs = hb_encoding.get_hbs_new(m);
    z3::expr forbid = prune::apply_prune_chain(prune_chain, hbs, m, o.print_pruning, o.out(), hb_encoding);

    if (o.machine && o.print_pruning >=1 ) {
      Z3_lbool forbid_value = Z3_get_bool_value(forbid.ctx(), forbid.simplify());
      if (forbid_value == Z3_L_UNDEF) {
        output::nf::result_type disj;
        output::nf::create_dnf(forbid, disj, hb_encoding);
        output::nf::print_one(cout, true, disj, true);
      }
    }
    if (o.print_rounds >=2) {
        o.out() << "Model:" << endl;
        o.out() << m << endl;
        o.out() << endl;
    }
    if (o.print_rounds >= 1) {
      o.out() << endl << endl;
    }
    //o.out() << forbid.simplify() << endl;
    // if( program->is_mm_declared() ) {
    //   z3::expr guarded_forbid = hb_encoding.mk_guarded_forbid_expr(hbs);
    //   sol_bad.add(guarded_forbid);
    // }else
    sol_bad.add(!forbid);
    result = result || forbid;
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
  
  if (o.print_output >= 1) {
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

      // check if all traces are bad
      /*sol_good.add(!result);
      if (sol_good.check() == z3::unsat)
        return trace_result::always;*/
      
      // simplify to summarise solutions
      z3::goal g(z3.c);
      g.add(result);
      z3::tactic simp(z3.c, "simplify");
      
      z3::tactic ctx_simp(z3.c, "ctx-simplify");
      
      z3::tactic final = simp & ctx_simp;
      
      z3::apply_result res = final.apply(g);
      
      g = res[0];
      result = z3.c.bool_val(true);
      for (unsigned i = 0; i < g.size(); i++)
        result = result && g[i]; // first thing we put in
        
      if (o.print_output >= 1) {
        o.out() << "Simplified formula:" << endl;
        o.out() << result << endl;
      }
      

      output.output(result);
      
      return_result = trace_result::depends;
      break;
    }
  }
  
  return return_result;
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
  cssa::variable_set po_vars = z3.get_variables(program->phi_po);
  cssa::variable_set vars = z3.get_variables(program->phi_vd && program->phi_pi && program->phi_pre && program->phi_prp);
  
  z3::expr_vector o(z3.c);
  z3::expr_vector n(z3.c);
    
  for (cssa::variable v: vars) {
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
    for (cssa::variable g : program->globals) {
      cssa::variable g1 = g + "#pre";
      cssa::variable g2 = g1 + "_g";
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
