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

%option noyywrap
%option nounput
%option yylineno
%option noinput
%option never-interactive

%x SMTM ASSUMEM

%{
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "parser.hpp"
#include <string>

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#endif

#define STOKEN( x ) ( new std::string( x ) )
#define ITOKEN( x ) ( atoi( x ) )
%}

string  \"[^\n"]+\"

ws      [ \t]+
nl      [\n]+


alpha   [A-Za-z]
dig     [0-9]
doublecolon "::"
dot "."
name    ({alpha}|{dig})({alpha}|{dig}|[_']|{doublecolon}|{dot})*
number  {dig}*

%%
%{
   int smt_nesting = 0;
   std::string* smt = nullptr;
%}

"("              { if (smt_expected) { BEGIN(SMTM); ++smt_nesting; smt = new std::string(); } else { return LPAREN; } }
<SMTM>"("        { ++smt_nesting; *smt+="("; }
<SMTM>")"        { if (--smt_nesting == 0) { BEGIN(INITIAL); yylval.sval = smt; return SMT; } else { *smt+=")"; } }
<SMTM>[^()]*     { *smt+= yytext; }
")"              { return RPAREN; }

{ws}            { /* ignore */ }
{nl}            { /* ignore */ }
"#".*           { /* ignore comment */ }
"instructions"  { return INSTRUCTIONS; }
"thread"        { return THREAD; }
"global"        { return GLOBAL; }
"assert"        { return ASSERT; }
"assume"        { return ASSUME; }
"havok"         { return HAVOK; }
"atomic"        { return AS; }
"order"         { return HB; }
"pre"           { return PRE;   }
"precondition"  { return PRE;   }
"ERROR"         { return ERROR; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
":"             { return COLON; }
";"             { return SEMICOLON; }
","             { return COMMA; }
"<"             { return LESS; }
">"             { return GREATER; }
{number}        { yylval.val = atoi(yytext); return NUMBER; }
{string}        { yylval.sval = new std::string(&yytext[1], strlen(yytext)-2);
                  return(STRING); }
{name}          { yylval.sval = new std::string(yytext, strlen(yytext));
                  return(NAME); } 
                  