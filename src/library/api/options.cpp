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
#include "helpers/helpers.h"
#include <iostream>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

using namespace tara;
using namespace tara::api;
using namespace std;

namespace po = boost::program_options;

options::options() : machine(false), prune_chain("data_flow,unsat_core"), _out(&cout)
{
    
}

bool options::has_sub_option( std::string s ) const {
  return helpers::exists( mode_options, s);
}

const std::string pruning_s = "pruning";
const std::string phi_s     = "phi"    ;
const std::string rounds_s  = "rounds" ;
const std::string output_s  = "output" ;
const std::string input_s   = "input"  ;

string string_of_print_options() {
  return "\"" +  pruning_s + "\",\"" + phi_s  + "\",\"" +
    rounds_s + "\",\"" + output_s + "\",\"" + input_s+ "\"";
}

void options::get_description(po::options_description& desc, po::positional_options_description& pd)
{
  std::string mm_select = "select a memory model ("+string_of_mm_names()+")";
  desc.add_options()
  ("hb-enc,e", po::value<string>(), "happens-before encoding (integer)")
  ("print,p", po::value<vector<string>>(), "print details about processing about a component (see readme)")
  ("prune,r", po::value<string>(), "select a chain of prune engines (see readme)")
  ("machine,m", po::bool_switch(&machine), "generate machine-readable output")
  ("mm,M", po::value<string>()->default_value("none"), mm_select.c_str() )
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
    arg_error(e.what());
  }
}


void options::interpret_options(po::variables_map& vm) {

  // verbosity options
  if( vm.count("print") ) {
    vector<string> pss = vm["print"].as<vector<string>>();
    for (string ps:pss) {
      boost::char_separator<char> sep(",");
      boost::tokenizer<boost::char_separator<char> > tok(ps, sep);
      for( std::string p : tok ) {
        if     ( p == pruning_s ) print_pruning++;
        else if( p == phi_s     ) print_phi++;
        else if( p == rounds_s  ) print_rounds++;
        else if( p == output_s  ) print_output++;
        else if( p == input_s   ) print_input++;
        else {
          std::string s = string_of_print_options();
          arg_error("Invalid printing type. Must be combination of "+s+".");
        }
      }
    }
  }

  if( vm.count("mm") ) {
    string _mode = seperate_option( vm["mm"].as<string>(), mm_options );
    mm = mm_of_string( _mode );
    if( mm == mm_t::wrong ) {
      std::string names = string_of_mm_names();
      arg_error( "Memory model must be one of: " + names );
    }
    // if (_mode == "sc")
    //   mm = mm_t::sc;
    // else if (_mode == "tso")
    //   mm = mm_t::tso;
    // else if (_mode == "pso")
    //   mm = mm_t::pso;
    // else if (_mode == "rmo")
    //   mm = mm_t::rmo;
    // else if (_mode == "alpha")
    //   mm = mm_t::alpha;
    // else if (_mode == "c11")
    //   mm = mm_t::c11;
    // else if (_mode == "none")
    //   mm = mm_t::none;
    // else {
    //   arg_error("Mode must be one of: \"sc\", \"tso\",\"pso\",\"rmo\",\"alpha\",\"c11\"");
    // }
  }

  if (vm.count("prune")) {
    prune_chain = vm["prune"].as<string>();
  }else
    {
    if( mm != mm_t::none ) {
      // default options for pruning under wmm
      prune_chain = "diffvar,unsat_core,remove_thin,remove_implied";
      if( mm == mm_t::c11 ) {
        prune_chain = "remove_non_relax_rf,unsat_core,remove_implied";
      }
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

