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
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <sstream>
#include <chrono>
#include <thread>

#include <api/trace_analysis.h>
#include <api/output/nf.h>
#include <api/output/smt.h>
#include <api/output/synthesis.h>
#include <api/output/fence_synth.h> // support for wmm
#include <api/output/bugs.h>
#include <helpers/helpers.h>
#include <helpers/z3interf.h>
#include "options_cmd.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace tara::api;

//---------------------------------------------
// gdb workaround, ask Ashutosh
ostream mout(cerr.rdbuf());

void copy_to_output(istream& in, ostream& display, ostream& file, const string& comment_symbol) {
  string line;
  while (getline(in, line)) {
    display << line << endl;
    file << comment_symbol << line << endl;
  }
}

chrono::steady_clock::time_point start_time;

bool print_metric = false;

tara::api::metric run_metric;

void finish_and_print(bool interrupted = false) {
  auto end_time = chrono::steady_clock::now();
  run_metric.time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
  if (print_metric)
    run_metric.print(cout, true, interrupted);
}

void handler(
  const boost::system::error_code& error,
  int signal_number) {
  cout << endl << "Caught interrupt" << endl;
  finish_and_print(true);
  exit(1);
}

void real_main(int argc, char **argv) {
  z3::context c;
  tara::helpers::z3interf z3(c);
  options_cmd o = options_cmd();


  boost::iostreams::stream< boost::iostreams::null_sink > nullOstream( ( boost::iostreams::null_sink() ) );
  if (o.machine)
    o._out = &nullOstream;

  boost::filesystem::path def_config("default.conf");
  if ( boost::filesystem::exists( def_config ) ) {
    o.parse_config(def_config);
  }

  if (!o.parse_cmdline(argc, argv)) return; // help was called

  if (o.machine)
    o._out = &nullOstream;

  boost::filesystem::path input_path(o.input_file);
  boost::filesystem::path dir = input_path.parent_path();
  // o.output_dir = dir/"output"/(input_path.stem())/"rounds";
  run_metric.filename = input_path.stem().string();
  print_metric = o.output_metric;


  if ( !boost::filesystem::exists( input_path ) )
  {
      arg_error("Input file " + o.input_file + " not found");
  }

  //boost::filesystem::remove_all(o.output_dir);
  //boost::filesystem::create_directories(o.output_dir);
  
  ofstream file_out;
  if (o.output_to_file) {
    //========================================================================
    //Ashutosh: test mangement
    //========================================================================
    //prepare option string
    // string ops="#!";
    // for( int i = 1; i < argc-1; i++ ) {
    //   if( strcmp( argv[i], "-f" ) ) {
    //     ops.append( " ");
    //     ops.append( argv[i] );
    //   }
    // }
    // ifstream file_in (o.input_file);
    // file_out.open(o.input_file+".new", ios::out | ios::trunc);
    // string line;
    // bool ops_found = false, output_found = false, output_finished = false;
    // while( getline(file_in, line) ) {
    //   if( !ops_found ) {
    //     file_out << line << endl;
    //     if( line == ops ) ops_found = true;
    //   }else if( !output_found ) {
    //     if( line.length() == 2 && line[0] == '#' && line[1] == '~' ) {
    //       file_out << line << endl;
    //       output_found = true;
    //     }
    //   }else if( !output_finished) {
    //     if( line.length() == 2 && line[0] == '#' && line[1] == '~' ) {
    //       // file_out << line << endl;
    //       output_finished = true;
    //     }
    //   }else{
    //     file_rest << line << endl;
    //   }
    // }
    //========================================================================

    // prepare output to the file if desired
    ifstream file_in (o.input_file);
    file_out.open(o.input_file+".new", ios::out | ios::trunc);
    string line;
    string seperator = "#####################";
    bool sep_found = false;
    while (getline(file_in, line)) {
      file_out << line << endl;
      if (line == seperator) {sep_found=true; break;}
    }
    file_in.close();
    if (!sep_found)
      file_out << seperator << endl;
  }
  stringstream resultbuf;

  start_time = chrono::steady_clock::now();
  trace_analysis ts(o,z3);
  // ts.input(o.input_file,o.mm);
  ts.input(o.input_file);
  ts.gather_statistics(run_metric);

  // TODO: Make this a seperate mode
  /*unordered_set< string >  ambigious;
  if (ts.check_ambigious_traces(ambigious)) {
    if (ambigious.size() > 0) {
      cerr << "WARNING: Some traces are good and bad depending on input variables:" << endl;
      cerr << "   ";
      for (string a : ambigious) 
        cerr << a << " ";
      cerr << endl;
    } else {
      cerr << "WARNING: Some traces are good and bad depending on other sources of non-determinism." << endl;
    }
  }*/
  
  bool synthesis = o.mode == modes::synthesis;
  bool fsynth = o.mode == modes::fsynth;
  bool bugs = o.mode == modes::bugs;
  switch (o.mode) {
    case modes::bugs:
    case modes::synthesis:
    case modes::fsynth:
    case modes::seperate: {
      bool silent = false;
      unique_ptr<output::output_base> output;
      if (o.mode_options.size()>0 && o.mode_options[0]=="smt") {
        output = unique_ptr<output::output_base>(new output::smt(o,z3));
      } if (synthesis||bugs||fsynth) {
        // bool verify = false;
        // bool print_nfs = false;
        // for (string p : o.mode_options) {
        //   if (p=="verify") verify = true;
        //   else if (p=="nfs") print_nfs = true;
        // }
        output =
            fsynth    ? unique_ptr<output::output_base>(new output::fence_synth(o,z3))
          : synthesis ? unique_ptr<output::output_base>( new output::synthesis(o,z3) )
          :             unique_ptr<output::output_base>( new output::bugs(o, z3)     );
      } else {
        // bool bad_dnf = false;
        // bool bad_cnf = false;
        // bool good_dnf = false;
        // bool good_cnf = false;
        // bool verify = false;
        // bool no_opt = false;
        // for (string p : o.mode_options) {
          // if      (p== "bad_dnf" ) bad_dnf = true;
          // else if (p== "bad_cnf" ) bad_cnf = true;
          // else if (p== "good_dnf") good_dnf = true;
          // else if (p== "good_cnf") good_cnf = true;
          // else if (p== "verify"  ) verify = true;
          // else if (p=="noopt") no_opt = true;
        //   else if (p=="silent") silent = true;
        // }
        silent = o.has_sub_option( "silent" );
        // if (!bad_dnf && !bad_cnf && !good_dnf && !good_cnf) { bad_dnf=true;}
        output = unique_ptr<output::output_base>( new output::nf(o,z3) );
      }
            
      trace_result res = ts.seperate(*output, run_metric);
      o.out() << endl;

      switch (res) {
        case trace_result::undecided: 
          resultbuf << "Solver undecided." << endl;
          if (o.machine)
            cout << "undecided" << endl;
          break;
        case trace_result::always: 
          resultbuf << "All traces are bad or infeasable." << endl;
          if (o.machine)
            cout << "true" << endl;
          break;
        case trace_result::never:
          resultbuf << "All traces are good or infeasable." << endl;
          break;
        case trace_result::depends:{
          output->gather_statistics(run_metric);
          resultbuf << "Final result" << endl;
          if (!silent) {         
            resultbuf << *output;
            if (o.machine)
              output->print(cout, true);
          }
          break;}
      }
      break;
    }
    
    case modes::as: {
      vector<tara::hb_enc::as> res_as;
      bool success_as = ts.atomic_sections(res_as);
      
      if (!success_as)
        resultbuf << "Atomic sections cannot fix the program" << endl;
      else {
        resultbuf << "Atomic sections that could fix the program" << endl;
        tara::helpers::print_vector(res_as, resultbuf);
      }
    }
  }
  

  
  copy_to_output(resultbuf, o.out(), file_out, "# ");
  if (o.output_to_file) {
    file_out.close();
    rename((o.input_file+".new").c_str(), o.input_file.c_str());
  }

  finish_and_print();
}

int main(int argc, char **argv) {
  boost::asio::io_service io_service;
  boost::asio::signal_set signals(io_service, SIGINT);
#ifdef __unix__
  signals.add(SIGHUP);
#endif
  signals.async_wait(handler);
  std::thread runner = std::thread(boost::bind(&boost::asio::io_service::run, &io_service));
  //runner.detach();
  
  int retval = 0;
  
  try {
    real_main(argc, argv);
  }
  catch (runtime_error e) {
    cerr << "Error: " << e.what() << endl;
    retval = 1;
  }
  
  io_service.stop();
  runner.join();
  return retval;
}

