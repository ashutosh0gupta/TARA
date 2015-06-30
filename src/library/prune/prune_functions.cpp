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

#include "prune_functions.h"
#include "prune/data_flow.h"
#include "prune/remove_implied.h"
#include "prune/unsat_core.h"
#include "helpers/helpers.h"
#include "api/options.h"

#include <boost/filesystem/fstream.hpp>

#include <boost/tokenizer.hpp>

using namespace tara::api;
using namespace tara::helpers;
using namespace std;

namespace tara {
namespace prune {
  
  void print_hbs(const list< z3::expr >& hbs, const hb_enc::encoding& hb_encoding, ostream& print_out) {
    for (auto& e : hbs) {
      auto hb = hb_encoding.get_hb(e);
      if (hb != nullptr)
        print_out << *hb << endl;
    }
  }
  
  z3::expr apply_prune_chain(const prune_chain& prune_chain, list< z3::expr >& hbs, const z3::model& m, unsigned print_pruning, ostream& print_out, const hb_enc::encoding& hb_encoding) {
    std::cout<<"\napply_prune_chain: 1\n";
        fflush(stdout);
    if (print_pruning >= 2) {
      print_out << "Original constraint for this ctex" << endl;
      print_hbs(hbs, hb_encoding, print_out);
    }
    
    for (unsigned i = 0; i < prune_chain.size(); i++) {
      //std::cout<<"\napply_prune_chain: 1.25\n";
        fflush(stdout);
      hbs = prune_chain[i]->prune(hbs, m);
            if (print_pruning >= 1) {
        print_out << "Constraint after " << prune_chain[i]->name() << endl;
        print_hbs(hbs, hb_encoding, print_out);
        /*boost::filesystem::ofstream dot_file(cmd_options->output_dir/("round" + to_string(cmd_options->round) + "_" + to_string(i) + "_" + prune_chain[i]->name() + ".dot"));
         prune_chain*[i]->program.print_dot(dot_file, res);
        dot_file.close();*/
      }
      //std::cout<<"\napply_prune_chain: 2\n";
        fflush(stdout);
    }
    
    z3::expr final = m.ctx().bool_val(true);
    for(auto r : hbs)
      final = final && r;
    return final;
  }
  
  bool build_prune_chain(const string& prune_order, prune_chain& prune_chain, const z3interf& z3, const cssa::program& program, z3::solver sol_good){
    if (prune_order=="" || prune_order=="none") {
      return true;
    }
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char> > tok(prune_order, sep);
    for (string p : tok) {
      unique_ptr<prune::prune_base> prune;
      if (p=="data_flow")
        prune = unique_ptr<prune::prune_base>(new data_flow(z3, program));
      else if (p=="remove_implied")
        prune = unique_ptr<prune::prune_base>(new remove_implied(z3, program));
      else if (p=="unsat_core")
        prune = unique_ptr<prune::prune_base>(new unsat_core(z3, program, sol_good));
      else {
        cerr << "Invalid prune engine. Must be one of \"data_flow\",\"remove_implied\",\"unsat_core\"." << endl;
        return false;
      }
      prune_chain.push_back(std::move(prune));
    }
    return true;
  }
}
}