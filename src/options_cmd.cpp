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

#include "options_cmd.h"
#include <iostream>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include "helpers/helpers.h"

using namespace std;
using namespace tara::api;

namespace po = boost::program_options;

const std::string mode_name = "mode";
const std::string bad_d =
"mode for succinctly representing bad executions.\n\
returns DNF of hb constraints (default)";
const std::string as_d =
"needed work (outdated)";
const std::string synth_d =
"mode for synthesis of sync premitives for sc memory";
const std::string fsynth_d =
"mode for synthesis of fences for weak memory,\n\
assumes input is good under sc";
const std::string bugs_d =
"mode for bugs localization";
const std::string smt2out_d =
"dumps smt formulas for good/bad formulas";
const std::string t_bad_d =
"checks if bad formula is sat and dumps witness";
const std::string t_good_d =
"checks if good formula is sat and dumps witness";

options_cmd::options_cmd()
  : options()
  , mode(modes::seperate)
  , output_to_file(false)
{
  // names for modes
  mode_names.insert( std::make_tuple( (modes::seperate), "bad", bad_d) );
  mode_names.insert( std::make_tuple( (modes::as), "as", as_d) );
  mode_names.insert( std::make_tuple( (modes::synthesis),"synthesis",synth_d));
  mode_names.insert( std::make_tuple( (modes::fsynth), "fsynth", fsynth_d) );
  mode_names.insert( std::make_tuple( (modes::bugs), "bugs", bugs_d ) );
// #ifndef NDEBUG
// todo: bring back this NDEBUG
  mode_names.insert( std::make_tuple( (modes::smt2out), "smt2out", smt2out_d ));
  mode_names.insert( std::make_tuple( (modes::t_bad), "test_bad", t_bad_d ));
  mode_names.insert( std::make_tuple( (modes::t_good), "test_good", t_good_d ));
// #endif
}

std::string options_cmd::string_mode_names() {
  bool first = true;
  std::stringstream ss;
  for( auto pr : mode_names) {
    if(first) first = false; else ss << ",";
    std::string s = std::get<1>(pr);
    ss << "\"" << s << "\"";
  }
  return ss.str();
}

std::string options_cmd::string_mode_discuss() {
  // bool first = true;
  std::stringstream ss;
  ss << "--" << mode_name << "\n------\n\n";
  for( auto pr : mode_names) {
    // if(first) first = false; else ss << ",";
    std::string s = std::get<1>(pr);
    std::string discuss = std::get<2>(pr);
    ss << "" << s << ":\n" << discuss << "\n";
  }
  ss << "\n";
  return ss.str();
}

void options_cmd::get_description_cmd(po::options_description& desc, po::positional_options_description& pd, std::string& extended)
{
  tara::api::options::get_description(desc, pd, extended);
  std::string m_name = mode_name + ",o";
  std::string moptions = "selects the mode of operation ("+string_mode_names()+")";
  desc.add_options()
  ("input,i", po::value(&input_file), "file to process")
  ("config,c", po::value<string>(), "a config file to parse")
  (m_name.c_str(), po::value<string>()->default_value("bad"), moptions.c_str() )
  ("ofile,f", po::bool_switch(&output_to_file), "append the output to the end of the input file")
  ("metric", po::bool_switch(&output_metric), "outputs statistics about the run")
  ("help,h", "produce help message")
  ("help-extended", "extended help for non-self-explanatory options")
  ;
  pd.add("input", 1);

  // add extended disccssions for non-trival options
  extended = extended + string_mode_discuss();
}


bool options_cmd::parse_cmdline(int argc, char** argv)
{
  po::variables_map vm;
  po::options_description desc;
  po::positional_options_description pd;
  std::string extended_discussion;
  get_description_cmd(desc, pd, extended_discussion);
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
      cout << desc << endl;
      return false;
    }
    if (vm.count("help-extended")) {
      cout << desc << endl;
      cout << "\n\nExtended help\n";
      cout << "=============\n\n";
      cout << extended_discussion;
      return false;
    }

    if (!vm.count("input")) {
      arg_error("No input specified");
    }
    if (vm.count("config")) {
      boost::filesystem::path path(vm["config"].as<string>());
      parse_config(path);
    }
    interpret_options(vm);

  } catch ( const boost::program_options::error& e ) {
    arg_error(e.what());
    return false;
  }
  return true;
}

void options_cmd::interpret_options(po::variables_map& vm) {
  options::interpret_options(vm);

  if (vm.count("mode")) {
    string _mode = seperate_option(vm["mode"].as<string>(), mode_options);
    bool none = true;
    for( auto pr : mode_names ) {
      if( std::get<1>(pr) == _mode ) {
        mode = std::get<0>(pr);
        none = false;
        break;
      }
    }
    if( none ) {
      std::string names = string_mode_names();
      arg_error("Mode must be one of: " + names);
    }
    if( mode == modes::fsynth ) {
      // if( mm != tara::mm_t::c11 )
        prune_chain = prune_chain + ",remove_non_cycled";
    }

  }

}



