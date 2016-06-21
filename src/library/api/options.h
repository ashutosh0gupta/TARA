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

#ifndef TARA_API_OPTIONS_H
#define TARA_API_OPTIONS_H

#include <boost/program_options.hpp>
#include <string>
#include <boost/filesystem.hpp>
#include "constants.h"

namespace tara {
namespace api {
    
enum class hb_types {
  integer
};

class options
{
public:
  options();
  hb_types hb_type = hb_types::integer;
  
  // verbosity options
  int print_input = 0;
  int print_pruning = 0;
  int print_phi = 0;
  int print_rounds = 0;
  int print_output = 0;
  boost::filesystem::path output_dir;
  
  bool machine; // produce machine-readable output
  
  std::string prune_chain;
  
  mm_t mm = mm_t::none;
  std::vector<std::string> mm_options;

  std::ostream* _out;
  inline std::ostream& out() const { return *_out; };
public: // information that are controlled by main
  unsigned round = 0;
  void parse_config(boost::filesystem::path filename);
protected:
  void get_description(boost::program_options::options_description& desc, boost::program_options::positional_options_description& pd);
  virtual void interpret_options(boost::program_options::variables_map& vm);
  
  std::string seperate_option(const std::string& to_parse, std::vector< std::string >& options); // parses the format xyz:bla,blub. xyz is returned and bla and blub added to the vector
};
}
}


#endif // TARA_API_OPTIONS_H
