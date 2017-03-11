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

  template <class T>
  using rand_iterator = std::iterator< std::random_access_iterator_tag, T >;

  //todo: should throw error if cycles detected
  //todo: record iterator
  template <class T, class n_iter> void
  topological_sort( const T head,
                    n_iter(succ_begin)(const T),
                    n_iter(succ_end)(const T),
                    std::vector< T >& ord_vec ) {
    // typedef std::iterator< std::random_access_iterator_tag, T > n_iter;
    ord_vec.clear();
    std::vector< std::tuple< const T, unsigned, n_iter, n_iter > > stack;
    auto tup = std::make_tuple(head, 0, succ_begin(head), succ_end(head) );
    stack.push_back( tup );
    std::map< T, unsigned > o_map;

    while( !stack.empty() ) {
      auto& tup = stack.back(); //std::cerr << e->name() << "\n";
      const T& n    = std::get<0>(tup);
      unsigned& max = std::get<1>(tup);
      n_iter& it    = std::get<2>(tup);
      n_iter& end   = std::get<3>(tup);
      assert( !helpers::exists( o_map, n ) );
      while( 1 ) {
        if( it != end ) {
          const T& np = *it;
          auto o_it = o_map.find( n );
          if( o_it == o_map.end() ) {
            unsigned m_new = o_it->second;
            if( m_new > max ) max = m_new;
            ++it;
          }else{
            stack.push_back( std::make_tuple(np, 0, succ_begin(np), succ_end(np)) );
            break;
          }
        }else{
          o_map[n] = max + 1;
          stack.pop_back();
          break;
        }
      }
    }

    std::sort( ord_vec.begin(), ord_vec.end(),
               [&](const T& x, const T& y) {return o_map.at(x) < o_map.at(y);});
  }

}}

#endif // GRAPH_UTILS_H
