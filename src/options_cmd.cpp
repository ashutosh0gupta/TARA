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

using namespace std;

namespace po = boost::program_options;

options_cmd::options_cmd() : tara::api::options(), mode(modes::seperate), output_to_file(false)
{
    
}

void options_cmd::get_description_cmd(po::options_description& desc, po::positional_options_description& pd)
{
  tara::api::options::get_description(desc, pd);
  desc.add_options()
  ("help,h", "produce help message")
  ("input,i", po::value(&input_file), "file to process")
  ("config,c", po::value<string>(), "a config file to parse")
  ("mode,o", po::value<string>()->default_value("seperate"), "selects the mode of operation (\"seperate\",\"as\",\"synthesis\",\"bugs\")")
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
      cerr << "No input specified" << endl;
      return false;
    }
    if (vm.count("config")) {
      boost::filesystem::path path(vm["config"].as<string>());
      if (!parse_config(path)) return false;
    }
    
    return interpret_options(vm);

  } catch ( const boost::program_options::error& e ) {
    std::cerr << e.what() << std::endl;
    return false;
  }

}

bool options_cmd::interpret_options(po::variables_map& vm) {
  if (!tara::api::options::interpret_options(vm))
    return false;
  
  if (vm.count("mode")) {
    string _mode = seperate_option(vm["mode"].as<string>(), mode_options);
    if (_mode == "seperate")
      mode = modes::seperate;
    else if (_mode == "as")
      mode = modes::as;
    else if (_mode == "synthesis")
      mode = modes::synthesis;
    else if (_mode == "bugs")
      mode = modes::bugs;
    else {
      cerr << "Mode must be one of: \"seperate\", \"lattice\", \"as\", \"synthesis\", \"bugs\"" << endl;
      return false;
    }
  }
  
  return true;
}



