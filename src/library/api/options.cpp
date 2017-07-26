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

std::string string_mm_discuss() {
  // bool first = true;
  std::stringstream ss;
  ss << "--mm \n----\n\n";
  ss << "choose memory model from the available models\n";
  ss << "sc is the default model\n";
  ss << "Model specific assumtions\n";
  ss << "c11: ctrc input is not allowed\n";
  ss << "arm8.2: (todo: ctrc add annotations to read/writes)\n";
  ss << "\n";
  return ss.str();
}


std::string string_print_discuss() {
  // bool first = true;
  std::stringstream ss;
  ss << "--print \n-------\n\n";
  ss << "repeated use of print options produces more detailed output\n";
  ss << "e.g. --print rounds,rounds,rounds\n";
  ss << "\n";
  return ss.str();
}


bool options::has_sub_option( std::string s ) const {
  return helpers::exists( mode_options, s);
}

const std::string pruning_s = "prune";
const std::string phi_s     = "phi"    ;
const std::string rounds_s  = "rounds" ;
const std::string output_s  = "output" ;
const std::string input_s   = "input"  ;

string string_of_print_options() {
  return "\"" +  pruning_s + "\",\"" + phi_s  + "\",\"" +
    rounds_s + "\",\"" + output_s + "\",\"" + input_s+ "\"";
}

void options::get_description(po::options_description& desc, po::positional_options_description& pd,std::string& extended)
{
  std::string mm_select = "select a memory model ("+string_of_mm_names()+")";
  std::string print_select = "print details of a component ("+string_of_print_options()+")";
  std::string unroll_count_str = "unroll count of loops (default "+
    std::to_string(loop_unroll_count) + ")";
  desc.add_options()
  ("hb-enc,e", po::value<string>(), "happens-before encoding (integer)")
  ("print,p", po::value<vector<string>>(), print_select.c_str() )
  ("prune,r", po::value<string>(), "select a chain of prune engines (see readme)")
  ("machine,m", po::bool_switch(&machine), "generate machine-readable output")
  ("mm,M", po::value<string>()->default_value("none"), mm_select.c_str() )
  ("unroll-loop,u", po::value<unsigned>(), unroll_count_str.c_str())
  ( "odir", po::value<string>()->default_value("/tmp/"),
    "choose directory for dumping temp files")
  ;
  pd.add("input", -1);

  // choosing memory models

  extended = extended + string_mm_discuss() + string_print_discuss();
}

void options::parse_config(boost::filesystem::path filename) {
  boost::filesystem::ifstream cfg_file(filename);
  po::variables_map vm;
  po::options_description desc;
  po::positional_options_description pd;
  std::string extended_discussion; // dummy option here
  get_description(desc, pd, extended_discussion);
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
  }

  if( vm.count("unroll-loop") ) {
    loop_unroll_count = vm["unroll-loop"].as<unsigned>();
  }

  if (vm.count("prune")) {
    prune_chain = vm["prune"].as<string>();
  }else{
    if( mm != mm_t::none ) {
      // default options for pruning under wmm
      prune_chain = "diffvar,unsat_core,remove_thin,remove_implied";
      if( mm == mm_t::c11 ) {
        prune_chain = "remove_non_relax_rf,unsat_core,remove_implied";
      }
      //prune_chain = "diffvar,unsat_core,remove_implied,remove_thin";
    }
  }

  if( vm.count("odir") ) {
    string outdir = vm["odir"].as<string>();
    if( outdir.back() != '/' )
      outdir += "/";
    if ( !boost::filesystem::exists( outdir ) ) {
      arg_error( "output directory " << outdir << " does not exists!!" );
    }
    output_dir = outdir;
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

