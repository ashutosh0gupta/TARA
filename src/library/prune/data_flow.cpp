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

#include "data_flow.h"
#include <queue>

using namespace tara::prune;
using namespace tara::helpers;
using namespace tara;
using namespace std;

data_flow::data_flow(const z3interf& z3, const tara::program& program) : prune_base(z3, program)
{
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
      throw std::runtime_error("data_flow analysis is unsupported for mms!!");
  }
  if( program.is_original() ) {// todo: remove this hack
    program_old = (const cssa::program*)(&program);
  }else{
    prune_data_flow_error( "new version of program not supported!!")
  }
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
}

string data_flow::name()
{
  return string("data_flow");
}


hb_enc::hb_vec data_flow::prune( const hb_enc::hb_vec& hbs,
                                      const z3::model& m )
{
  throw std::runtime_error("data_flow is out of sync needs work!!");
  // todo : copy the following code into the new interface
  return hbs;
}

list<z3::expr> data_flow::prune(const list<z3::expr>& hbs, const z3::model& m)
{
  list<z3::expr> pi_hbs;
  queue<string> follow;
  // init with the variables in the assertions
  for(std::shared_ptr<tara::instruction> instr : program_old->assertion_instructions) {
    for (string v : instr->variables_read)
      follow.push(v);
  }
  
  unordered_set<Z3_ast> seen_expressions; // to prevent duplicates
  unordered_set<std::string> seen_variable; // to prevent duplicates
  
  // queue that does a breath-first search over the dependencies and builds up a conjunct
  while(true) {
    string v;
    bool found = false;
    while (!follow.empty()) {
      v = follow.front();
      follow.pop();
      if (get<1>(seen_variable.insert(v))) { // do until we insert a new element
        found = true;
        break;
      }
    }
    if (!found) break;
    // if this is a local variable
    auto local = program_old->variable_written.find(v);
    if (local != program_old->variable_written.end()) {
      std::shared_ptr<tara::instruction> instr = local->second;
      for (string v : instr->variables_read)
        follow.push(v);
    } else {
      auto global = program_old->pi_functions.find(v);
      assert(global!=program_old->pi_functions.end());
      vector<cssa::pi_function_part> pi_parts = global->second;
#ifndef NDEBUG
      bool part_matched = false;
#endif
      for (cssa::pi_function_part pi_part : pi_parts) {
        //cout << pi_part.hb_exression << endl;
        if (m.eval(pi_part.hb_exression).get_bool()) {
#ifndef NDEBUG
          assert(part_matched == false);
          part_matched = true;
#endif
          // decompose the conjunction into seperate
          vector<z3::expr> conjs = z3.decompose(pi_part.hb_exression, Z3_OP_AND);
          // check if we already added this expression
          for (z3::expr c:conjs) {
            auto inserted = seen_expressions.insert(c);
            if (get<1>(inserted)) {
              insert_disjunct(c, pi_hbs, m);
            }
          }
          for (string vn : pi_part.variables) {
            //cout << v << " -> " << vn << endl;
            follow.push(vn);
          }
#ifdef NDEBUG
          break;
#endif
        }
      }
      assert (part_matched);
    }
  }
  return pi_hbs;
}

void data_flow::insert_disjunct(const z3::expr& disjunction, std::list< z3::expr >& hbs, const z3::model& m)
{
#ifndef NDEBUG
    int counter = 0;
#endif
    for (auto d : z3interf::decompose(disjunction, Z3_OP_OR)) {
      if (m.eval(d).get_bool()) {
        hbs.push_back(d);
#ifndef NDEBUG
        counter++;
#else
        break;
#endif
      }
    }
    assert(counter==1);
}

