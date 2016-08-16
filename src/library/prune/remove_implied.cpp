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

#include "remove_implied.h"
#include<cassert>

using namespace tara;
using namespace tara::helpers;
using namespace tara::prune;

using namespace std;

remove_implied::remove_implied(const z3interf& z3, const tara::program& program) : prune_base(z3, program)
{
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso() || program.is_mm_rmo() || program.is_mm_alpha()){
    }else{
      throw std::runtime_error("remove_implied does not support memory model!");
    }
  }
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
}

string remove_implied::name()
{
  return string("remove_implied");
}

bool remove_implied::compare_events( const hb_enc::se_ptr e1,
                                     const hb_enc::se_ptr e2 ) {
  // check if these edge is between the same thread
  if( e1 == e2 ) return true;
  if( e1->get_tid() != e2->get_tid() ) return false;

  switch( program.get_mm() ) {
  case mm_t::sc:  return compare_sc_events(  e1, e2 ); break;
  case mm_t::tso: return compare_tso_events( e1->e_v, e2->e_v ); break;
  case mm_t::pso: return compare_pso_events( e1->e_v, e2->e_v ); break;
  case mm_t::rmo: return compare_rmo_events( e1->e_v, e2->e_v ); break;
  case mm_t::alpha: return compare_alpha_events( e1->e_v, e2->e_v ); break;
  default:
      throw std::runtime_error("remove_implied does not support memory model!");
  }
}

bool remove_implied::compare_sc_events( const hb_enc::se_ptr e1,
                                        const hb_enc::se_ptr e2 ) {
  auto loc1 = e1->e_v;
  auto loc2 = e2->e_v;
  // check if the other is from earlier instruction
  if( loc1->instr_no < loc2-> instr_no ) return true;
  // if they are from same instruction then they must be Rd-Wr
  if( loc1->instr_no == loc2->instr_no ) {
    if( loc1->is_read && !loc2->is_read ) return true;
  }
  return false;
}

bool remove_implied::compare_tso_events( const hb_enc::location_ptr loc1,
                                         const hb_enc::location_ptr loc2 ) {
  // check if the other one is more specific
  if( !loc1->is_read && loc2->is_read ) return false;

  if( loc1->instr_no < loc2-> instr_no ) return true;
  if( loc1->instr_no == loc2->instr_no ) {
    if( loc1->is_read && !loc2->is_read ) return true;
  }
  return false;
}


bool remove_implied::compare_pso_events( const hb_enc::location_ptr loc1,
                                         const hb_enc::location_ptr loc2 ) {
  // check if the other one is more specific
  if( !loc1->is_read && loc2->is_read ) return false;
  if( !loc1->is_read && !loc2->is_read)
  {
    if( loc1->prog_v_name != loc2->prog_v_name )
      {
        return false;
      }
  }
  if( loc1->instr_no < loc2-> instr_no ) return true;
  if( loc1->instr_no == loc2->instr_no ) {
    // if( loc1 == loc2 ) return true;
    if( loc1->is_read && !loc2->is_read ) return true;
  }
  return false;
}


bool remove_implied::compare_rmo_events( const hb_enc::location_ptr loc1,
                                         const hb_enc::location_ptr loc2 ) {
  if( loc2->is_read ) return false;
  // loc2 is write now onwards
  if ( loc1->is_read ) {
    // loc1 read - loc2 write
    // todo: check if dep relation contains intr internal relations
    // todo: add control dep also
    for( auto it1=program.data_dependency.begin();
         it1!=program.data_dependency.end();it1++ ) {
      if( (it1->first)->e_v == loc2 ) {
        //todo: check this change from loc to e_v
        for(auto it2=it1->second.begin(); it2!=it1->second.end(); it2++ ) {
          if((*it2).e->e_v==loc1) {
            //todo : check conditional depedency!!!
            return true;
          }
        }
      }
    }
  }

  if ( loc1->prog_v_name != loc2->prog_v_name ) return false;

  if( loc1->instr_no < loc2-> instr_no ) return true;
  return false;
}

bool remove_implied::compare_alpha_events( const hb_enc::location_ptr loc1,
					   const hb_enc::location_ptr loc2 ) {

    if ( loc1->prog_v_name != loc2->prog_v_name ) return false;
    if( !loc1->is_read && loc2->is_read ) return false;

    if( loc1->instr_no < loc2-> instr_no ) return true;
    if( loc1->instr_no == loc2->instr_no ) {
	if( loc1->is_read && !loc2->is_read ) return true;
  }

    return false;
}
list< z3::expr > remove_implied::prune( const list< z3::expr >& hbs,
                                        const z3::model& m )
{
  list<z3::expr> result = hbs;
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    for (auto it = result.begin() ; it != result.end(); ) {
      bool remove = false;
      for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
        // ensure that we do not compare with ourselves
        if (it != it2) {
          // find duplicate
          if ((Z3_ast)*it == (Z3_ast)*it2) {
            remove = true;
            break;
          }
          unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
          unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
          assert( hb1 && hb2 );
          assert( hb1->e1 && hb1->e2 && hb2->e1 && hb2->e2 );
          if( compare_events( hb1->e1, hb2->e1 ) &&
              compare_events( hb2->e2, hb1->e2 ) ) {
            remove = true;
            break;
          }
          // if( compare_events( hb1->loc1, hb2->loc1 ) &&
          //     compare_events( hb2->loc2, hb1->loc2 ) ) {
          //   remove = true;
          //   break;
          // }
        }
      }
      if (remove) {
        //cerr << *it << endl;
        it = result.erase(it);
      }else 
        ++it;
    }
    
    return result;
  }
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
  for (auto it = result.begin() ; it != result.end(); ) {
    bool remove = false;
    for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
      // ensure that we do not compare with ourselves
      if (it != it2) {
        // find duplicate
        if ((Z3_ast)*it == (Z3_ast)*it2) {
          remove = true;
          break;
        }
        unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
        unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
        assert (hb1 && hb2);
        // check if these edge is between the same thread
        if (hb1->loc1->thread == hb2->loc1->thread && hb1->loc2->thread == hb2->loc2->thread) {
          // check if the other one is more specific
          if (hb1->loc1->instr_no <= hb2->loc1->instr_no && hb1->loc2->instr_no >= hb2->loc2->instr_no)
          {
            remove = true;
            break;
          }
        }
      }
    }
    if (remove) {
      //cerr << *it << endl;
      it = result.erase(it);
    }else 
      ++it;
  }
  return result;
}

