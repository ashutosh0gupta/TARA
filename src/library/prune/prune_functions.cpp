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
#include "prune/diffvar.h"
#include "prune/data_flow.h"
#include "prune/remove_implied.h"
#include "prune/unsat_core.h"
#include "prune/remove_thin.h"
#include "prune/remove_non_cycled.h"
#include "prune/remove_non_relax_rf.h"
#include "helpers/helpers.h"
#include "api/options.h"

#include <boost/filesystem/fstream.hpp>

#include <boost/tokenizer.hpp>

using namespace tara::api;
using namespace tara::helpers;
using namespace std;

namespace tara {
namespace prune {

  // void print_hbs( const list< z3::expr >& hbs,
  //                 const hb_enc::encoding& hb_encoding, ostream& print_out) {
  //   for (auto& e : hbs) {
  //     auto hb = hb_encoding.get_hb(e);
  //     if (hb != nullptr) {
  //       print_out << *hb << endl;
  //     }else{
  //       print_out << "---- null ----" << endl;
  //     }
  //   }
  // }

  void print_hbs( const hb_enc::hb_vec& hbs, ostream& print_out ) {
    for (auto& hb : hbs) {
      if (hb != nullptr) {
        print_out << *hb << endl;
      }else{
        print_out << "---- null ----" << endl;
      }
    }
  }

  void
  apply_prune_chain( const prune_chain& prune_chain,
                     const api::options& o,
                     const z3::model& m,
                     hb_enc::hb_vec& hbs
                     // unsigned print_pruning,
                     // ostream& print_out,
                     // const hb_enc::encoding& hb_encoding
                     ) {
    unsigned print_pruning = o.print_pruning;
    ostream& print_out = o.out();

    if (print_pruning >= 2) {
      print_out << "Atoms in the theory solvers for this ctex:" << endl;
      print_hbs(hbs, print_out );
    }

    for( unsigned i = 0; i < prune_chain.size(); i++ ) {
      hbs = prune_chain[i]->prune( hbs, m );
      if (print_pruning >= 1) {
        print_out << "--- Atoms left after " << prune_chain[i]->name() << ":"
                  << endl;
        print_hbs(hbs, print_out);
      }
    }
    return;
  }

  bool build_prune_chain(const string& prune_order, prune_chain& prune_chain, const z3interf& z3, const tara::program& program, z3::solver sol_good){
    if (prune_order=="" || prune_order=="none") {
      return true;
    }
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char> > tok(prune_order, sep);
    for (string p : tok) {
      unique_ptr<prune::prune_base> prune;
      if ( p == "data_flow" )
        prune = unique_ptr<prune::prune_base>(new data_flow(z3, program));
      else if ( p == "remove_implied" )
        prune = unique_ptr<prune::prune_base>(new remove_implied(z3, program));
      else if ( p == "unsat_core" )
        prune = unique_ptr<prune::prune_base>(new unsat_core(z3, program, sol_good));
      else if ( p == "diffvar" )
        prune = unique_ptr<prune::prune_base>(new diffvar(z3, program));
      else if ( p == "remove_thin" )
	prune = unique_ptr<prune::prune_base>(new remove_thin(z3, program));
      else if ( p == "remove_non_cycled" )
	prune = unique_ptr<prune::prune_base>(new remove_non_cycled(z3, program));
      else if ( p == "remove_non_relax_rf" )
        prune =unique_ptr<prune::prune_base>(new remove_non_relax_rf(z3,program));
      else {
        cerr << "Invalid prune engine. Must be one of \"data_flow\",\"remove_implied\",\"unsat_core\",\"remove_thin\",\"remove_non_cycled\",\"remove_non_relax_rf\"." << endl;
        return false;
      }
      prune_chain.push_back(std::move(prune));
    }
    return true;
  }
}
}
