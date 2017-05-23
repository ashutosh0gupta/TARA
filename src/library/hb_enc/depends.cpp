/*
 * Copyright 2017, TIFR
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


#include "hb_enc/symbolic_event.h"
#include "hb_enc/encoding.h"
// #include "hb_enc/hb.h"

using namespace tara;
using namespace tara::hb_enc;
using namespace tara::helpers;

using namespace std;

//------------------------------------------------------------------------
// depends set utilities

void
hb_enc::pointwise_and( const hb_enc::depends_set& dep_in,
                       z3::expr cond,
                       hb_enc::depends_set& dep_out ) {
  dep_out.clear();
  for( const hb_enc::depends& d : dep_in ) {
    z3::expr c = d.cond && cond;
    c = c.simplify();
    dep_out.insert( hb_enc::depends( d.e, c ));
  }
}

void
hb_enc::join_depends_set( const std::vector<hb_enc::depends_set>& dep,
                          const std::vector<z3::expr>& conds,
                          hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  if( num >= 1 ) pointwise_and( dep.at(0), conds.at(0), result );
  for( unsigned i = 1 ; i < num ; i++ ) {
    hb_enc::depends_set tmp;
    pointwise_and( dep.at(i), conds.at(i), tmp );
    join_depends_set( tmp, result );
  }
}

void
hb_enc::join_depends_set( const std::vector<hb_enc::depends_set>& dep,
                          hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  if ( num == 1 )
    result = dep.at(0);
  else if ( num == 2 )
    join_depends_set( dep.at(0), dep.at( 1 ), result );
  else if ( num > 2 ) {
    for ( unsigned i = 0 ; i < num ; i++ ) {
      join_depends_set( dep.at( i ), result );
    }
  }
}

//todo : make depends set efficient
//       make it a well implemented class

void hb_enc::join_depends_set( const hb_enc::se_ptr& e1, const z3::expr cond1,
                               depends_set& set ) {
  z3::expr cond = cond1;
  for( auto it1 = set.begin(); it1 != set.end(); it1++) {
    const hb_enc::depends& dep2 = *it1;
    hb_enc::se_ptr e2 = dep2.e;
    if( e1 == e2 ) {
      cond = ( cond || dep2.cond );
      cond = cond.simplify();
      set.erase(it1);
      break;
    }
  }
  set.insert( hb_enc::depends( e1, cond ) );

}

void hb_enc::join_depends_set( const hb_enc::depends& dep,
                               hb_enc::depends_set& set ) {
  join_depends_set( dep.e, dep.cond, set );
}

void hb_enc::join_depends_set( const hb_enc::depends_set& dep0,
                               hb_enc::depends_set& dep1 ) {
  for( auto element0 : dep0 )
    hb_enc::join_depends_set( element0, dep1 );
  // tara::debug_print( std::cout, dep1 );
}


void hb_enc::join_depends_set( const hb_enc::depends_set& dep0,
                               const hb_enc::depends_set& dep1,
                               hb_enc::depends_set& set ) {
  set = dep1;
  join_depends_set( dep0, set );
}

void hb_enc::meet_depends_set( const hb_enc::se_ptr& e1, const z3::expr cond1,
                               depends_set& set ) {
  z3::expr cond = cond1;
  for( auto it1 = set.begin(); it1 != set.end(); it1++) {
    const hb_enc::depends& dep2 = *it1;
    hb_enc::se_ptr e2 = dep2.e;
    if( e1 == e2 ) {
      cond = ( cond && dep2.cond );
      cond = cond.simplify();
      set.erase(it1);
      break;
    }
  }
  set.insert( hb_enc::depends( e1, cond ) );
}

void hb_enc::meet_depends_set( const hb_enc::depends& dep,
                               hb_enc::depends_set& set ) {
  meet_depends_set( dep.e, dep.cond, set );
}

void hb_enc::meet_depends_set( const hb_enc::depends_set& dep0,
                               hb_enc::depends_set& dep1 ) {
  for( auto element0 : dep0 )
    hb_enc::meet_depends_set( element0, dep1 );
}

void hb_enc::meet_depends_set( const hb_enc::depends_set& dep0,
                               const hb_enc::depends_set& dep1,
                               hb_enc::depends_set& set ) {
  set = dep1;
  meet_depends_set( dep0, set );
}


void hb_enc::meet_depends_set( const std::vector<hb_enc::depends_set>& dep,
                               hb_enc::depends_set& result ) {
  result.clear();
  unsigned num = dep.size();
  if( num >= 1 ) result =  dep.at(0);
  for( unsigned i = 1 ; i < num ; i++ )
    meet_depends_set( dep.at(i), result );
  // return data_dep_ses;
}

//-------------------------------------------------------------------------

hb_enc::depends
hb_enc::pick_maximal_depends_set( hb_enc::depends_set& set ) {
  //todo : get depends set ordered
  unsigned ord = 0;
  depends d = *set.begin(); //todo : dummy initialization
  for( auto& dep : set ) {
    if( dep.e->get_topological_order() > ord ) d = dep;
  }
  set.erase( d );
  return d;
}
