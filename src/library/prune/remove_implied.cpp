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

remove_implied::remove_implied(const z3interf& z3, const cssa::program& program) : prune_base(z3, program)
{
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  if( program.is_mm_declared() ) {
    if( program.is_mm_tso() || program.is_mm_sc() || program.is_mm_pso() || program.is_mm_rmo()){
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

bool remove_implied::compare_events( const hb_enc::location_ptr loc1,
                                     const hb_enc::location_ptr loc2 ) {
  // check if these edge is between the same thread
  if( loc1->thread != loc2->thread ) return false;

  switch( program.get_mm() ) {
  case mm_t::sc:  return compare_sc_events(  loc1, loc2 ); break;
  case mm_t::tso: return compare_tso_events( loc1, loc2 ); break;
  case mm_t::pso: return compare_pso_events( loc1, loc2 ); break;
  case mm_t::rmo: return compare_rmo_events( loc1, loc2 ); break;
  default:
      throw std::runtime_error("remove_implied does not support memory model!");
  }
}

bool remove_implied::compare_sc_events( const hb_enc::location_ptr loc1,
                                        const hb_enc::location_ptr loc2 ) {
  // check if the other is from earlier instruction
  if( loc1->instr_no < loc2-> instr_no ) return true;
  // if they are from same instruction then they must be Rd-Wr
  if( loc1->instr_no == loc2->instr_no ) {
    if( loc1 == loc2 ) return true;
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
    if( loc1 == loc2 ) return true;
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
    if( loc1 == loc2 ) return true;
    if( loc1->is_read && !loc2->is_read ) return true;
  }
  return false;
}


bool remove_implied::compare_rmo_events( const hb_enc::location_ptr loc1,
                                         const hb_enc::location_ptr loc2 ) {
  // check if the other one is more specific
  if( !loc1->is_read && loc2->is_read ) return false;
  if( loc1->is_read && loc2->is_read ) return false;
  if( !loc1->is_read && !loc2->is_read)
  {
    if( loc1->prog_v_name != loc2->prog_v_name )
      {
        return false;
      }
  }

  if( loc1->is_read && !loc2->is_read)
  {
    if( loc1->prog_v_name != loc2->prog_v_name )
      {
        return false;
      }
  //  }
  // if(loc1->is_read)
  // {
    // todo: create map between locs -> locs
  for(auto it1=program.dependency_relation.begin();
      it1!=program.dependency_relation.end();it1++)
    {
      if( (it1->first)->loc == loc2 )
        {
          for(auto it2=it1->second.begin(); it2!=it1->second.end(); it2++ )
            {
              if((*it2)->loc==loc1)
                {
                  return true;
                }
            }
        }
    }
  }


  if( loc1->instr_no < loc2-> instr_no ) return true;
  if( loc1->instr_no == loc2->instr_no ) {
    if( loc1 == loc2 ) return true;
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
          assert (hb1 && hb2);
          if( compare_events( hb1->loc1, hb2->loc1 ) &&
              compare_events( hb2->loc2, hb1->loc2 ) )
            {
              remove = true;
              break;
            }
        }
      }
      if (remove) {
        //cerr << *it << endl;
        it = result.erase(it);
      }else 
        ++it;
    }
    
    // if( program.is_mm_sc() ) {
    //    for (auto it = result.begin() ; it != result.end(); ) {
    //      bool remove = false;
    //      for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
    //        // ensure that we do not compare with ourselves
    //        if (it != it2) {
    //          // find duplicate
    //          if ((Z3_ast)*it == (Z3_ast)*it2) {
    //            remove = true;
    //            break;
    //          }
    //          unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
    //          unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
    //          assert (hb1 && hb2);
    //          if( compare_sc_events( hb1->loc1, hb2->loc1 ) &&
    //              compare_sc_events( hb2->loc2, hb1->loc2 ) ) {
    //                remove = true;
    //                break;
    //          }
    //        }
    //      }
    //      if (remove) {
    //        //cerr << *it << endl;
    //        it = result.erase(it);
    //      }else
    //        ++it;
    //    }
    //  }else if( program.is_mm_tso() ) {
    //   for (auto it = result.begin() ; it != result.end(); ) {
    //     bool remove = false;
    //     for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
    //       // ensure that we do not compare with ourselves
    //       if (it != it2) {
    //         // find duplicate
    //         if ((Z3_ast)*it == (Z3_ast)*it2) {
    //           remove = true;
    //           break;
    //         }
    //         unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
    //         unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
    //         assert (hb1 && hb2);
    //          if( compare_tso_events( hb1->loc1, hb2->loc1 ) &&
    //              compare_tso_events( hb2->loc2, hb1->loc2 ) )
    //             {
    //               remove = true;
    //               break;
    //             }
    //       }
    //     }
    //     if (remove) {
    //       //cerr << *it << endl;
    //       it = result.erase(it);
    //     }else 
    //       ++it;
    //   }
    // }
    // else if(program.is_mm_pso())
    // {
    //   for (auto it = result.begin() ; it != result.end(); ) {
    //       bool remove = false;
    //       for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
    //         // ensure that we do not compare with ourselves
    //         if (it != it2) {
    //           // find duplicate
    //           if ((Z3_ast)*it == (Z3_ast)*it2) {
    //             remove = true;
    //             break;
    //           }
    //           unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
    //           unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
    //           assert (hb1 && hb2);
    //           if( compare_pso_events( hb1->loc1, hb2->loc1 ) &&
    //               compare_pso_events( hb2->loc2, hb1->loc2 ) ) {
    // 	                  remove = true;
    // 	                  break;
    //           }
    //         }
    //       }
    //       if (remove) {
    //         //cerr << *it << endl;
    //         it = result.erase(it);
    //       }else
    //         ++it;
    //   }
    // }
    // else if(program.is_mm_rmo())
    // {

    //   for (auto it = result.begin() ; it != result.end(); ) {
    //     bool remove = false;
    //     for (auto it2 = result.begin() ; it2 != result.end(); ++it2) {
    //       // ensure that we do not compare with ourselves
    //       if (it != it2) {
    //         // find duplicate
    //         if ((Z3_ast)*it == (Z3_ast)*it2) {
    //           remove = true;
    //           break;
    //         }
    //         unique_ptr<hb_enc::hb> hb1 = hb_enc.get_hb(*it);
    //         unique_ptr<hb_enc::hb> hb2 = hb_enc.get_hb(*it2);
    //         assert (hb1 && hb2);
    //         if( compare_rmo_events( hb1->loc1, hb2->loc1 ) &&
    //             compare_rmo_events( hb2->loc2, hb1->loc2 ) )
    //           {
    //             remove = true;
    //             break;
    //           }
    //         // }
    //       }
    //     }
    //     if (remove) {
    //       //cerr << *it << endl;
    //       it = result.erase(it);
    //     }else
    //       ++it;
    //   } 
    // }
    // else{
    //   throw std::runtime_error("unsupported memory model");
    // }
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

