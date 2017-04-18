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

options_cmd::options_cmd() : options(), mode(modes::seperate), output_to_file(false)
{
  // names for modes
  mode_names.insert( std::make_pair( (modes::seperate), "seperate") );
  mode_names.insert( std::make_pair( (modes::as), "as") );
  mode_names.insert( std::make_pair( (modes::synthesis), "synthesis") );
  mode_names.insert( std::make_pair( (modes::fsynth), "fsynth") );
  mode_names.insert( std::make_pair( (modes::bugs), "bugs") );
}

std::string options_cmd::string_mode_names() {
  bool first = true;
  std::stringstream ss;
  for( auto pr : mode_names) {
    if(first) first = false; else ss << ",";
    ss << "\"" << pr.second << "\"";
  }
  return ss.str();
}

void options_cmd::get_description_cmd(po::options_description& desc, po::positional_options_description& pd)
{
  tara::api::options::get_description(desc, pd);
  std::string moptions = "selects the mode of operation ("+string_mode_names()+")";
  desc.add_options()
  ("help,h", "produce help message")
  ("input,i", po::value(&input_file), "file to process")
  ("config,c", po::value<string>(), "a config file to parse")
    ("mode,o", po::value<string>()->default_value("seperate"), moptions.c_str() )
  ("ofile,f", po::bool_switch(&output_to_file), "append the output to the end of the input file")
  ("metric", po::bool_switch(&output_metric), "outputs statistics about the run")
  ;
  pd.add("input", 1);
}


bool options_cmd::parse_cmdline(int argc, char** argv)
{
  po::variables_map vm;
  po::options_description desc;
  po::positional_options_description pd;
  get_description_cmd(desc, pd);
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
      cout << desc << endl;
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
      if( pr.second == _mode ) {
        mode = pr.first;
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

    // if (_mode == "seperate")
    //   mode = modes::seperate;
    // else if (_mode == "as")
    //   mode = modes::as;
    // else if (_mode == "synthesis")
    //   mode = modes::synthesis;
    // else if (_mode == "wmm_synthesis" && _mode == "fsynth") {
    //   mode = modes::fsynth;
    //   prune_chain = prune_chain + ",remove_non_cycled";
    // }else if (_mode == "bugs")
    //   mode = modes::bugs;
    // else {
    //   std::string names = string_mode_names();
    //   arg_error("Mode must be one of: " + names);
    // }
  }

}



