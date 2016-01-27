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

#include <iostream>
#include <stdio.h>
#include <string>
#include "helpers/helpers.h"

std::string int_to_string(int num) {
  //std::string str;
  switch(num)
    {
    case 0:  return "0";  break;
    case 1:  return "1";  break;
    case 2:  return "2";  break;
    case 3:  return "3";  break;
    case 4:  return "4";  break;
    case 5:  return "5";  break;
    case 6:  return "6";  break;
    case 7:  return "7";  break;
    case 8:  return "8";  break;
    case 9:  return "9";  break;
    case 10: return "10"; break;
    default: 
      std::cout<<"Sorry invalid type\n";
      exit(0);
    }
}

  //--------------------------------------------------------------------------
  //support for gdb
void tara::input::debug_print( const variable_set& set, std::ostream& out) {
  for( auto& v : set ) {
    out << v.name << " ";
  }
  out << "\n";
}

void tara::cssa::debug_print( const variable_set& set, std::ostream& out) {
  for( auto& v : set ) {
    out << v.name << " ";
  }
  out << "\n";
}

  //--------------------------------------------------------------------------
