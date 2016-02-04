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

%error-verbose
%locations
%expect 0

%code requires {
  #include <stdio.h>
  #include <string>
  #include "input/parse.h"
  #include "constants.h"
  //#include <parser.hpp>
  
  extern bool smt_expected;
}

%union
{
  std::string *sval;
  int val; 
  std::pair<std::string*,std::string*>* pval;
  char sym;
  tara::input::string_set *str_set;
  tara::instruction_type instruction_type;
};

%code {
  int yylex (void);
  void yyerror (tara::input::parse&, char const *);
  
  int thread = -1;
  int instruction = 0;
  std::string thread_name;
  bool smt_expected = true;
}


%parse-param {tara::input::parse& in}

%token INSTRUCTIONS THREAD GLOBAL ERROR PRE LOCAL ASSERT ASSUME HAVOK  AS HB POST
%token FENCE SYNC LWSYNC DMB BARRIER BARRIER_A BARRIER_B
%token COLON COMMA SEMICOLON LBRACE RBRACE LBRACKET RBRACKET LPAREN RPAREN LESS GREATER
%token <val> NUMBER
%token <sval> STRING SMT NAME

%type <sval> name_number  instr_name name
%type <str_set> names havok
%type <pval> variable 
%type <instruction_type> instr_type fence

%start File
%%
File: globals pre threads atomic_sections happens_befores post;

globals: GLOBAL COLON gnames;

gnames: variable { in.addGlobal($1); }
      | variable { in.addGlobal($1); } gnames
;

pre:  PRE COLON SMT { in.addPrecondition($3); }
    | { in.addPrecondition(nullptr); }
;

threads:   thread
         | thread threads;

thread:   threadh COLON instrs
        | threadh locals COLON instrs

threadh:   THREAD name_number { thread++; instruction = 0; thread_name = *$2; in.addThread($2); }

locals: LBRACE lnames RBRACE;

lnames: variable { in.addLocal(thread, $1); }
      | variable { in.addLocal(thread, $1); } lnames
;

variable: NAME { $$ = new std::pair<std::string*,std::string*>($1, new std::string("int")); }
      | NAME COLON NAME { $$ = new std::pair<std::string*,std::string*>($1, $3); }
      | NAME COLON SMT { $$ = new std::pair<std::string*,std::string*>($1, $3); }
;

instrs: instr_name havok instr_type SMT { in.addInstruction(thread, $1, $4, $2, $3); } instrs
    | fence_instr instrs |
;

fence_instr:  instr_name fence {in.addInstruction(thread, $1, NULL, NULL, $2); }
        ;
instr_name: name COLON { instruction++;  $$ = $1; }
      | { instruction++;  $$ = new std::string(thread_name+std::to_string(instruction)); }
;

instr_type:  { $$ = tara::instruction_type::regular; }
      | ASSERT { $$ = tara::instruction_type::assert; }
      | ASSUME { $$ = tara::instruction_type::assume; }
;

fence:  FENCE  { $$ = tara::instruction_type::fence;  }
      | SYNC   { $$ = tara::instruction_type::sync;   }
      | LWSYNC { $$ = tara::instruction_type::lwsync; }
      | DMB    { $$ = tara::instruction_type::dmb;    }
      | BARRIER   { $$ = tara::instruction_type::barrier;   }
      | BARRIER_A { $$ = tara::instruction_type::barrier_a; }
      | BARRIER_B { $$ = tara::instruction_type::barrier_b; }
;

havok:   HAVOK {smt_expected = false;} LPAREN names RPAREN { smt_expected=true; $$ = $4; }
       | { $$ = new tara::input::string_set(); }
;

names : { $$ = new tara::input::string_set(); }
      | NAME names { $$ = $2; $$->insert(*$1); delete $1; }
      | NAME COMMA names { $$ = $3; $$->insert(*$1); delete $1; }
;

atomic_sections : 
      | AS COLON ases
;

happens_befores : 
      | HB COLON hbs
;

post:  POST COLON SMT { in.addPostcondition($3); }
    | { in.addPostcondition(nullptr); }
;

ases: LBRACKET name name RBRACKET { in.addAtomicSection($2,$3);  } ases
      |
;

hbs:  NAME LESS NAME { in.addHappensBefore($1,$3);  } hbs
      |
;

name :  name_number { $$ = $1; }
      | name_number LBRACKET name_number RBRACKET { $$ = new std::string(*$1 + "[" + *$3 + "]"); delete $1; delete $3; }
;

// could be name or number
name_number: NAME { $$=$1; }
  | NUMBER { $$ = new std::string(std::to_string($1)); }
;
