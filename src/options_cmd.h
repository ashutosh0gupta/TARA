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

#ifndef OPTIONS_CMD_H
#define OPTIONS_CMD_H

#include <boost/program_options.hpp>
#include <string>
#include <boost/filesystem.hpp>
#include <api/options.h>

enum class modes {
  seperate,
  as,
  synthesis,
  fsynth,//wmm support
  bugs,
  smt2out,
};

class options_cmd : public tara::api::options
{
public:
  options_cmd();
  std::string input_file;
  modes mode;
  bool output_to_file;
  bool output_metric;

  std::set< std::tuple<modes, std::string, std::string> > mode_names;

public: // information that are controlled by main
  bool parse_cmdline(int argc, char **argv);
private:

  void get_description_cmd(boost::program_options::options_description& desc,
                           boost::program_options::positional_options_description& pd,
                           std::string& extended);
  virtual void interpret_options(boost::program_options::variables_map& vm) override;
  std::string string_mode_names();
  std::string string_mode_discuss();
};



#endif // OPTIONS_H
