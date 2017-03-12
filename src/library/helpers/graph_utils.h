/*
 * Copyright 2017, TIFR, India
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


#ifndef GRAPH_UTILS_H
#define GRAPH_UTILS_H

#include "helpers/helpers.h"
#include <iterator>

namespace tara {
namespace helpers {

  //todo: should throw error if cycles detected
  //todo: record iterator
  template <class T, class n_iter> void
  topological_sort( T h,
                    std::function<n_iter(T)> succ_begin,
                    std::function<n_iter(T)> succ_end,
                    std::vector< T >& ord_vec ) {
    ord_vec.clear();
    std::vector< std::tuple< T, unsigned, n_iter, n_iter > > stack;
    stack.push_back( std::make_tuple( h, 0, succ_begin(h), succ_end(h) ) );
    std::map< T, unsigned > o_map;

    while( !stack.empty() ) {
      auto& tup     = stack.back();
      T& n          = std::get<0>(tup);
      unsigned& max = std::get<1>(tup);
      n_iter& it    = std::get<2>(tup);
      n_iter& end   = std::get<3>(tup); //std::cerr << n << "\n";
      assert( !helpers::exists( o_map, n ) );
      while( 1 ) {
        if( it != end ) {
          const T& np     = *it;
          auto o_it = o_map.find( np );
          if( o_it != o_map.end() ) {
            unsigned m_new = o_it->second;
            if( m_new > max ) max = m_new;
            ++it;
          }else{
            stack.push_back(std::make_tuple(np,0,succ_begin(np),succ_end(np)));
            break;
          }
        }else{
          o_map[n] = max + 1;
          // std::cout << max+1 << "\n";
          ord_vec.push_back(n);
          stack.pop_back();
          break;
        }
      }
    }

    std::sort( ord_vec.begin(), ord_vec.end(),
               [&](const T& x, const T& y) {return o_map.at(x) > o_map.at(y);});
  }

}}

#endif // GRAPH_UTILS_H
