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

#include "remove_thin.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_thin::remove_thin(const z3interf& z3, const cssa::program& program) : prune_base(z3, program)
{
  if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso() || program.is_mm_rmo() || program.is_mm_alpha()){
  }else{
    throw std::runtime_error("remove_thin does not support memory model!");
  }
}

string remove_thin::name()
{
  return string("remove_thin");
}

list< z3::expr > remove_thin::prune( const list< z3::expr >& hbs,
				     const z3::model& m )
{
	assert( program.is_mm_declared() );
        list<z3::expr> result = hbs;
	for (auto it = result.begin() ; it != result.end(); ) {
	  bool remove = false;
	  unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
	  auto& l1 = hb1->loc1;
	  auto& l2 = hb1->loc2;
	  if(( l1->event_name.find("__thin__") != std::string::npos) && (l2->event_name.find("__thin__") != std::string::npos)) remove = true; 
	  if( remove ) it = result.erase(it); else ++it;
	 
	}
	 return result;
}


