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

#include "constants.h"
#include "options.h"
#include <iostream>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include "arg_exception.h"

using namespace tara;
using namespace tara::api;
using namespace std;

namespace po = boost::program_options;

options::options() : machine(false), prune_chain("data_flow,unsat_core"), _out(&cout)
{
    
}

void options::get_description(po::options_description& desc, po::positional_options_description& pd)
{
  desc.add_options()
  ("hb-enc,e", po::value<string>(), "happens-before encoding (integer)")
  ("print,p", po::value<vector<string>>(), "print details about processing about a component (see readme)")
  ("prune,r", po::value<string>(), "select a chain of prune engines (see readme)")
  ("machine,m", po::bool_switch(&machine), "generate machine-readable output")
  ("mm,M", po::value<string>()->default_value("none"), "selects the memory model (\"sc\",\"tso\",\"pso\")")
  ;
  pd.add("input", -1);
}

void options::parse_config(boost::filesystem::path filename) {
  boost::filesystem::ifstream cfg_file(filename);
  po::variables_map vm;
  po::options_description desc;
  po::positional_options_description pd;
  get_description(desc, pd);
  po::notify(vm);
  try {
    po::store(po::parse_config_file(cfg_file, desc, false), vm);
    po::notify(vm);
    
    interpret_options(vm);
    
  } catch ( const boost::program_options::error& e ) {
    throw arg_exception(e.what());
  }
}


void options::interpret_options(po::variables_map& vm) {
  
  
  // verbosity options
  if (vm.count("print")) {
    vector<string> pss = vm["print"].as<vector<string>>();
    for (string ps:pss) {
      boost::char_separator<char> sep(",");
      boost::tokenizer<boost::char_separator<char> > tok(ps, sep);
      for (string p : tok) {
        if (p=="pruning")
          print_pruning++;
        else if (p=="phi")
          print_phi++;
        else if (p=="rounds")
          print_rounds++;
        else if (p=="output")
          print_output++;
        else if (p=="input")
          print_input++;
        else {
          throw arg_exception("Invalid printing type. Must be combination of \"pruning\",\"phi\",\"rounds\",\"output\",\"input\".");
        }
      }
    }
  }

  if (vm.count("mm")) {
    string _mode = seperate_option(vm["mm"].as<string>(), mm_options);
    if (_mode == "sc")
      mm = mm_t::sc;
    else if (_mode == "tso")
      mm = mm_t::tso;
    else if (_mode == "pso")
      mm = mm_t::pso;
    else if (_mode == "rmo")
      mm = mm_t::rmo;
    else if (_mode == "alpha")
      mm = mm_t::alpha;
    else if (_mode == "none")
      mm = mm_t::none;
    else {
      throw arg_exception("Mode must be one of: \"sc\", \"tso\",\"pso\",\"rmo\",\"alpha\"");
    }
  }

  if (vm.count("prune")) {
    prune_chain = vm["prune"].as<string>();
  }else
    {
    if( mm != mm_t::none ) {
      // default options for pruning under wmm
      prune_chain = "diffvar,unsat_core,remove_thin,remove_implied";
      //prune_chain = "diffvar,unsat_core,remove_implied,remove_thin";
    }
    }
}


string options::seperate_option(const string& to_parse, vector< string >& options)
{
  size_t seperator = to_parse.find(':');
  if (seperator == string::npos)
    return to_parse;
  
  string value = to_parse.substr(0, seperator);
  string opts = to_parse.substr(seperator+1);
  boost::char_separator<char> sep(",");
  boost::tokenizer<boost::char_separator<char> > tok(opts, sep);
  for (string p : tok) {
    options.push_back(p);
  }
  return value;
}

