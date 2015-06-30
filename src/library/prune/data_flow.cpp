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

data_flow::data_flow(const z3interf& z3, const cssa::program& program) : prune_base(z3, program)
{}

string data_flow::name()
{
  return string("data_flow");
}


list<z3::expr> data_flow::prune(const list<z3::expr>& hbs, const z3::model& m)
{
  list<z3::expr> pi_hbs;
  queue<string> follow;
  std::cout<<"\ndata-flow: prune 1\n";
        fflush(stdout);
  // init with the variables in the assertions
  for(std::shared_ptr<cssa::instruction> instr : program.assertion_instructions) {
    for (string v : instr->variables_read)
    {
       follow.push(v);
       //std::cout<<"\nv:\t"<<v<<"\n";
    } 

  }
  //std::cout<<"\napply_prune_chain: 2\n";
        fflush(stdout);
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
    //std::cout<<"\napply_prune_chain: 3\n";
        fflush(stdout);
    if (!found) break;
    // if this is a local variable
    auto local = program.variable_written.find(v);
    //std::cout<<"\nv:\t"<<v<<"\n";
    //std::cout<<"\nlocal: \t"<<program.variable_written[v].instr<<"\n";
    //std::cout<<"\napply_prune_chain: 3.1\n";
        fflush(stdout);
    if (local != program.variable_written.end()) {
      std::shared_ptr<cssa::instruction> instr = local->second;
      //std::cout<<"\ninstr: \t"<<instr<<"\n";
      //std::cout<<"\napply_prune_chain: 3.2\n";

        fflush(stdout);
      for (string v : instr->variables_read)
        follow.push(v);
      //std::cout<<"\napply_prune_chain: 4\n";
        fflush(stdout);
    } else {
      //std::cout<<"\ninstr\n"<<local->second;
      //std::cout<<"\napply_prune_chain: 3.3\n";
        fflush(stdout);
      auto global = program.pi_functions.find(v);
      std::cout<<"\nglobal v:\t"<<v<<"\n";
      assert(global!=program.pi_functions.end());
      
      //std::cout<<"\napply_prune_chain: 3.4\n";
        fflush(stdout);




      vector<cssa::pi_function_part> pi_parts = global->second;





#ifndef NDEBUG
      bool part_matched = false;
#endif
      //std::cout<<"\napply_prune_chain: 5\n";
        fflush(stdout);
      for (cssa::pi_function_part pi_part : pi_parts) {
        std::cout <<"\nhb_expression:\t"<< pi_part.hb_exression << endl;
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
  //std::cout<<"\napply_prune_chain: 10\n";
        fflush(stdout);
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

