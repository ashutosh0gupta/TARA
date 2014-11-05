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

#include <boost/filesystem.hpp>

#include "parse.h"
#include "input_exception.h"
#include "constants.h"
#include "helpers/helpers.h"

#include <iostream>

#ifdef _MSC_VER

#endif

using namespace std;
using namespace tara::input;

parse::parse()
{

}

extern int yylineno;

bool parse_error = false;
void yyerror(parse& in, char const* err) {
  cerr << "Parse error at line " << yylineno << endl;
  cerr << err << endl;
  parse_error = true;
}

int yyparse(parse&);
extern FILE* yyin;

program parse::parseFile(const char* file)
{
  yyin = fopen( file, "r" );
  if (yyin == 0) {
    std::stringstream fmt;
	boost::filesystem::path full_path(boost::filesystem::current_path());
	fmt << "File " << file << " in directory " << full_path << " not accessible";
    throw input_exception(fmt.str().c_str());
  }
  parse in;
  int res = yyparse(in);
  fclose(yyin);
  if (res!=0)
    throw input_exception("Parse error");
  return std::move(in.p);
}

void parse::addThread(string* name) {
  p.threads.push_back(thread(*name));
  delete name;
}

void parse::addInstruction(int thread, string* name, string* command, string_set* havok_vars, tara::instruction_type t)
{
  string c = *command;
  variable_set havok_variables;
  
  p.threads[thread].instrs.push_back(make_shared<instruction_str>(*name, c, t, *havok_vars));
  delete command;
  delete name;
  delete havok_vars;
}

variable parse::translate_type(string& name, string& type) {
  if (type=="bv16")
    return variable(name, data_type::bv16);
  if (type=="bv32")
    return variable(name, data_type::bv32);
  if (type=="bv64")
    return variable(name, data_type::bv64);
  if (type=="int" || type=="Int")
    return variable(name, data_type::integer);
  if (type=="bool" || type=="Bool")
    return variable(name, data_type::boolean);
  if (type=="real" || type=="Real")
    return variable(name, data_type::real);
  return variable(name, type);
}

void parse::addLocal(int thread, pair<string*,string*>* local)
{
  p.threads[thread].locals.insert(translate_type(*get<0>(*local), *get<1>(*local)));
  delete get<0>(*local);
  delete get<1>(*local);
  delete local;
}


void parse::addGlobal(pair<string*,string*>* global)
{
  p.globals.insert(translate_type(*get<0>(*global), *get<1>(*global)));
  delete get<0>(*global);
  delete get<1>(*global);
  delete global;
}

void parse::addPrecondition(string* pre)
{
  string a;
  if (pre==nullptr) {
    a = "true";
  } else {
    a = *pre;
  }
  p.precondition = make_shared<instruction_str>(string("precondition"), a, instruction_type::assume, string_set());
  delete pre;
}

void parse::addAtomicSection(string* loc1, string* loc2)
{
  p.atomic_sections.push_back(make_pair(*loc1,*loc2));
  delete loc1;
  delete loc2;
}

void parse::addHappensBefore(string* loc1, string* loc2)
{
  p.happens_befores.push_back(make_pair(*loc1,*loc2));
  delete loc1;
  delete loc2;
}
