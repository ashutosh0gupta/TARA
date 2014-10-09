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

#include "metric.h"
#include <boost/algorithm/string/replace.hpp>
#include <locale> 
#include <sstream>
#include <iomanip>

using namespace std;

namespace tara {
namespace api {
  
  struct myseps : numpunct<char> { 
    /* use space as separator */
    char do_thousands_sep() const { return ';'; } 
    
    /* digits are grouped by 3 digits each */
    string do_grouping() const { return "\3"; }
  };
  
  template <typename T>
  string NumberToString ( T Number )
  {
    ostringstream ss;
    ss.imbue(std::locale(std::locale(), new myseps));
    ss.setf( std::ios::fixed, std:: ios::floatfield );
    ss << setprecision(1);
    ss << Number;
    string s = ss.str();
    boost::replace_all(s, ";", "\\;");
    return s;
  }
  
  template <typename T>
  string NumberToStringkM ( T Number )
  {
    ostringstream ss;
    ss.imbue(std::locale(std::locale(), new myseps));
    ss.setf( std::ios::fixed, std:: ios::floatfield );
    ss << setprecision(1);    
    if (Number>1000000) {
      ss << (double(Number)/1000000);
      ss << "M";
    } else if (Number>1000) {
      ss << (double(Number)/1000);
      ss << "k";
    } else {
      ss << Number;
    }
    string s = ss.str();
    boost::replace_all(s, ";", "\\;");
    return s;
  }
  
std::ostream& operator<< (std::ostream& stream, const metric& metric) {
  metric.print(stream);
  return stream;
}

void metric::print(ostream& stream, bool latex, bool interrupted) const
{
  string conf;
  if (data_flow) conf+="D";
  if (unsat_core) conf+="U";
  
  string filename = this->filename;
  //boost::replace_all(filename, "_", "\\_");
  
  string greater;
  string latex_greater;
  if (interrupted) {
    greater = "> ";
    latex_greater = ">";
  }
  
  const string term = " ";
  const string lterm = " \\\\ ";
  stream << "Time: " << greater << time << " ms" << endl;
  stream << "Rounds: " << greater << rounds << endl;
  stream << "Average round time: " << ((double)sum_round_time)/1000/rounds << endl;
  stream << "Threads/Instructions: " << threads << "/" << instructions << endl;
  stream << "Reads/Average Writes to: " << shared_reads << "/" << NumberToString(float(sum_reads_from)/shared_reads) << endl;
  stream << "Amount of HBs: " << greater << sum_hbs << endl;
  
  // calculate time
  string timef = NumberToString(time);
  string unit = "ms";
  
  if (time>1000) {
    timef = NumberToString(double(time)/1000);
    unit = "s";
  }
  if (time>60000) {
    timef = NumberToString(double(time)/60000);
    unit = "min";
  }

  
  if (latex) {    
    stream << left << setw(15) << filename << "";
    stream << "% " << conf << " file_name - " << this->filename << endl;
    
    stream << left << setw(15) << filename << "";
    stream << right << setw(5) << NumberToString(threads) + "/" + NumberToString(instructions) << " ";
    stream << right << setw(7) << NumberToString(shared_reads) + "/" + NumberToString(float(sum_reads_from)/shared_reads) << " " ;
    stream << "% " << conf << " file_info - " << this->filename << endl;
    
    stream << term << setw(6) << latex_greater + NumberToStringkM(rounds) << "% " << NumberToString(rounds);
    stream << " % " << conf << " rounds_count - " << this->filename << endl;
    
    if (interrupted)
      stream << term << setw(6) << "TO";
    else
    stream << term << setw(6) << latex_greater + timef + unit;
    stream << "% " << conf << " total_time - " << this->filename << endl;
    
    if (interrupted) {
      stream << term << setw(10) << "TO";
      } else
        stream << term << setw(10) << latex_greater + NumberToStringkM(disjuncts_after) + "/" + NumberToString(float(sum_hbs_after)/disjuncts_after) << "% " << NumberToString(disjuncts_after) << "/" << NumberToString(float(sum_hbs_after)/disjuncts_after);
    stream << "% " << conf << " hbs_after - " << this->filename << endl;
    
    stream << term << additional_info;
    stream << "% " << conf << " extra_info - " << this->filename << endl;
    
  }
}


}}