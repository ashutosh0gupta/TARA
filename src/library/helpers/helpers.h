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

#ifndef HELPERS_H
#define HELPERS_H

#include "helpers_exception.h"
#include <string>
#include <unordered_set>
#include <set>
#include <list>
#include <vector>
#include <ostream>
#include <boost/concept_check.hpp>
#include <z3++.h>
#include <ctype.h>
#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#ifdef EXTERNAL_VERSION
#define triggered_at ""
#else
#define triggered_at " (triggered at " <<  __FILE__ << ", " << __LINE__ << ")"
#endif


namespace tara {
class tara_exception : public std::runtime_error
{
public:
  tara_exception(const char* what) : runtime_error(what) {}
  tara_exception(const std::string what) : runtime_error(what.c_str()) {}
};
}

#ifndef NDEBUG
#define issue_error( ss )  { std::cerr << ss.str() << "\n"; assert( false );}
#else
#define issue_error( ss )  { throw tara::tara_exception( ss.str() ); }
#endif

#define tara_error( M, S ) { std::stringstream ss;                   \
                             ss << "# tara" << M << " Error - " << S   \
                                << triggered_at << std::endl;        \
                             issue_error( ss ); }


#define cinput_error( S )            tara_error( "::cinput",             S )
#define program_error( S )           tara_error( "::program",            S )
#define trace_error( S )             tara_error( "::trace",              S )
#define wmm_error( S )               tara_error( "::wmm",                S )
#define barrier_synthesis_error( S ) tara_error( "::barrier_synthesis",  S )
#define bugs_error( S )              tara_error( "::bugs",               S )
#define prune_error( S )             tara_error( "::prune::data_flow",   S )
#define prune_data_flow_error( S )   tara_error( "::prune",              S )
#define cssa_error( S )              tara_error( "::cssa",               S )
#define hb_enc_error( S )            tara_error( "::hb_enc",             S )
#define z3interf_error( S )          tara_error( "::z3interf",           S )

#define tara_warning( M, S ) { std::cerr << "# tara" << M << " Warning: " << S \
                                      << std::endl; }

#define cinput_warning( S )       tara_warning( "::cinput", S )


#define COMPARE_TAIL( A, B, Tail ) ( A < B || ( A == B && ( Tail ) ) )
#define COMPARE_LAST( A, B )       ( A < B )

#define COMPARE_PTR1( A, B, F ) COMPARE_LAST( A->F, B->F )
#define COMPARE_PTR2( A, B, F1, F2 ) COMPARE_TAIL( A->F1, B->F1, \
        COMPARE_PTR1( A, B, F2 ) )
#define COMPARE_PTR3( A, B, F1, F2, F3 ) COMPARE_TAIL( A->F1, B->F1, \
        COMPARE_PTR2( A, B, F2, F3 ) )
#define COMPARE_PTR4( A, B, F1, F2, F3, F4 ) COMPARE_TAIL( A->F1, B->F1, \
        COMPARE_PTR3( A, B, F2, F3, F4 ) )

#define COMPARE_OBJ1( A, B, F ) COMPARE_LAST( A.F, B.F )
#define COMPARE_OBJ2( A, B, F1, F2 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ1( A, B, F2 ) )
#define COMPARE_OBJ3( A, B, F1, F2, F3 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ2( A, B, F2, F3 ) )
#define COMPARE_OBJ4( A, B, F1, F2, F3, F4 ) COMPARE_TAIL( A.F1, B.F1, \
        COMPARE_OBJ3( A, B, F2, F3, F4 ) )


namespace std {
  template <>
  struct hash<z3::expr>
  {
    std::size_t operator()(const z3::expr& k) const
    {
      using std::size_t;
      using std::hash;
      
      
      return (hash<Z3_ast>()((Z3_ast)k));
    }
  };
}

namespace tara {
  
namespace input {
  enum class data_type {
    bv32,
    bv16,
    bv64,
    integer,
    boolean,
    real,
    smt, // an smt string
  };
  
  struct variable {
    std::string name;
    data_type type;
    std::string smt_type; // in case it is not one of the above types
    operator std::string() const { return name;}
    variable(std::string name, data_type type) : name(name), type(type) {}
    variable(std::string name, std::string type) : name(name), type(data_type::smt), smt_type(type) {}
  };
  
  struct variable_hash {
    size_t operator () (const variable &v) const { return std::hash<std::string>()(v.name); }
  };
  
  struct variable_equal : std::binary_function <variable,variable,bool> {
    bool operator() (const variable& x, const variable& y) const {
      return std::equal_to<std::string>()(x.name, y.name);
    }
  };
  
  typedef std::unordered_set<variable, variable_hash, variable_equal> variable_set;
  typedef std::unordered_set<std::string> string_set;

  //--------------------------------------------------------------------------
  //support for gdb
  void debug_print( const variable_set& , std::ostream& out);
  //--------------------------------------------------------------------------
}

namespace cssa {
  struct variable {
    std::string name;
    z3::sort sort;
    operator std::string() const { return name;}
    variable(std::string name, z3::sort sort) : name(name), sort(sort) {}
    variable(std::string name, z3::context& ctx) : name(name), sort(z3::sort(ctx)) {}
    variable(z3::context& ctx) : sort(z3::sort(ctx)) {}
    friend variable operator+ (const variable& v, const std::string& str) {
      return variable(v.name+str, v.sort);
    }
    friend variable operator+ (const std::string& str, const variable& v) {
      return variable(str+v.name, v.sort);
    }
    
    operator z3::expr() const {
      return sort.ctx().constant(name.c_str(), sort);
    }
    
    friend std::ostream& operator<< (std::ostream& stream, const variable& var) {
      stream << var.name;
      return stream;
    }
    bool operator==(const variable& rhs) const {
      return std::equal_to<std::string>()(this->name, rhs.name);
    }
    bool operator!=(const variable& rhs) const {
      return !(*this==rhs);
    }
    bool operator<( const variable& rhs) const {
      return this->name < rhs.name;
    }
  };
  
  
  
  struct variable_hash {
    size_t operator () (const variable &v) const { return std::hash<std::string>()(v.name); }
  };
  
  struct variable_equal : std::binary_function <variable,variable,bool> {
    bool operator() (const variable& x, const variable& y) const {
      return x==y;
    }
  };
  
  typedef std::unordered_set<variable, variable_hash, variable_equal> variable_set;

  //--------------------------------------------------------------------------
  //support for gdb
  void debug_print( const variable_set& , std::ostream& out);
  //--------------------------------------------------------------------------

}

namespace helpers {
  
inline cssa::variable get_unprimed(const cssa::variable& variable) {
  //converting the primed to unprimed by removing the dot
  if (variable.name[variable.name.size()-1] != '.') {
    return variable;
  } else {
    std::string v1 = variable.name.substr(0, variable.name.size()-1);
    return cssa::variable(v1, variable.sort);
  }
}

inline std::string get_unprimed(const std::string& variable) {
  if (variable[variable.size()-1] != '.') {
    return variable;
  } else {
    std::string v1 = variable.substr(0, variable.size()-1);
    return v1;
  }
}


inline bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
  //--------------------------------------------------------------------------
  // added for wmm code support -- should be eventually removed
  //--------------------------------------------------------------------------

inline bool check_correct_global_variable(const cssa::variable& variable, std:: string prog_name)
{
  unsigned vnum_len = variable.name.size();
  unsigned prog_len = prog_name.size();
  if( prog_len >= vnum_len) return false;
  for(unsigned i = prog_len; i < vnum_len;i++) {
    if( !isdigit(variable.name[i]) ) return false;
  }
  for(unsigned i=0; i< prog_len; i++ ) {
    if( variable.name[i] != prog_name[i] ) return false;
  }
  return true;
}

  inline cssa::variable find_orig_variable( const cssa::variable& var,
                                             const cssa::variable_set& vars ) {
    for( auto& v : vars ) {
      if( check_correct_global_variable( var, v.name ) ) {
        return v; // TODO: This return value may trigger copy operation; 
                  // resolve this issue
      }
    }
    throw helpers_exception("variable is not find in a set!");
  }

  //--------------------------------------------------------------------------
  // End of wmm support code
  //--------------------------------------------------------------------------

inline bool is_primed(const std::string& variable) {
  return (variable[variable.size()-1] == '.');
}

inline std::string get_before_hash(const std::string& variable) {
  std::string result;
  for (char c:variable) {
    if (c=='#')
      break;
    result.push_back(c);
  }
  return result;
}

template< class Key >
bool exists( const std::vector<Key>& v, const Key& k ) {
  return std::find( v.begin(), v.end(), k ) != v.end();
  // return set1.find( k ) !=  set1.end();
}


template< class Key >
bool exists( const std::set<Key>& set1, const Key& k ) {
  return set1.find( k ) !=  set1.end();
}

template< class Key,class cmp >
bool exists( const std::set<Key,cmp>& set1, const Key& k ) {
  return set1.find( k ) !=  set1.end();
}

  // Insert second set inside the first set
template< class Key >
Key pick( std::set<Key>& doner ) {
    Key x = *doner.begin();
    doner.erase( x );
    return x;
}

template< class Key, class cmp >
Key pick( std::set<Key, cmp>& doner ) {
    Key x = *doner.begin();
    doner.erase( x );
    return x;
}

template< class Key, class cmp >
Key pick_and_move( std::set<Key,cmp>& doner,
                   std::set<Key,cmp>& receiver ) {
  Key x = pick( doner );
  receiver.insert( x );
  return x;
}

template< class Key >
Key pick_and_move( std::set<Key>& doner,
                   std::set<Key>& receiver ) {
  Key x = pick( doner );
  receiver.insert( x );
  return x;
}

template< class Key >
void set_insert( std::set<Key>& set1, const std::set<Key>& set2 ) {
  set1.insert( set2.begin(), set2.end() );
}

template< class Key >
void set_intersection( const std::set<Key>& set_src,
                       std::set<Key>& set ) {
  auto j =  set.begin();
  for (; j!=set.end(); ) {
    if ( exists( set_src, *j ) )
      j++;
    else
      j = set.erase(j);
  }
}

template< class Key >
void set_intersection( const std::vector< std::set<Key> >& vec_set,
                       std::set<Key>& set ) {
  set.clear();
  if( vec_set.empty() ) return;
  set = vec_set[0];
  for( unsigned i = 1; i < vec_set.size(); i++ ) {
    set_intersection( vec_set[i], set );
  }
}

template< class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>>
void set_insert( std::unordered_set<Key, Hash, Pred>& set1,
                 const std::unordered_set<Key, Hash, Pred>& set2 ) {
  set1.insert( set2.begin(), set2.end() );
}

template <class Key, class Hash = std::hash<Key>,
          class Pred = std::equal_to<Key>>
std::unordered_set<Key, Hash, Pred>
set_union( const std::unordered_set<Key, Hash, Pred>& set1,
           const std::unordered_set<Key, Hash, Pred>& set2 ) {
  std::unordered_set<Key, Hash, Pred> res = set1;
  res.insert(set2.begin(), set2.end());
  return res;
}

  template < class Key, class Hash = std::hash<Key>,
             class Pred = std::equal_to<Key>>
  void print_set( std::ostream& out,
                  const std::unordered_set<Key, Hash, Pred>& set1 ) {
    for (Key c : set1) {
      out << c << " ";
    }
    out << std::endl;
  }

template < class Key,
           class Hash = std::hash< Key >,
           class Pred = std::equal_to< Key > >
std::unordered_set<Key, Hash, Pred>
set_intersection( const std::unordered_set<Key, Hash, Pred>& set1,
                  const std::unordered_set<Key, Hash, Pred>& set2 ) {
  std::unordered_set<Key, Hash, Pred> result;
  for (auto& e : set1) {
    if (set2.find(e) != set2.end())
      result.insert(e);
  }
  return result;
}

template < class Key,
           class Hash = std::hash< Key >,
           class Pred = std::equal_to< Key > >
bool
intersection_nonempty( const std::unordered_set<Key, Hash, Pred>& set1,
                       const std::unordered_set<Key, Hash, Pred>& set2 ) {
  for (auto& e : set1) {
    if (set2.find(e) != set2.end())
      return true;
  }
  return false;
}

template <class content>
void print_vector(const std::vector<content>& v, std::ostream& out) {
  for(content c: v)
    out << c << std::endl;
}

template <class content>
void remove_duplicates(std::list<content>& v) {
  for(auto it = v.begin(); it!=v.end(); it++) {
    auto j = it;
    for (j++; j!=v.end(); ) {
      if (*j==*it)
        j = v.erase(j);
      else
        j++;
    }
  }
}

template <class content>
void print_list(const std::list<content>& v, std::ostream& out) {
  for(content c: v)
    out << c << std::endl;
}

template <class content>
void print_array(content v[], unsigned length, std::ostream& out) {
  out << "[ ";
  for(unsigned i=0; i<length; i++)
    out << v[i] << " ";
  out << "]" << std::endl;
}

template <class T1>
inline std::pair<T1,T1> swap_pair(std::pair<T1,T1> p) {
  T1 h = p.first;
  p.first = p.second;
  p.second = h;
  return p;
}

template <typename Iterator, class T>
bool last_element(Iterator element, const std::list<T>& list) {
  return ++element == list.end();
}

template <typename Iterator, class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>>
bool last_element(Iterator element, const std::unordered_set<Key, Hash, Pred>& list) {
  return ++element == list.end();
}

template <class content>
bool list_contains(const std::list<content>& v, const content& element) {
  for(content c: v)
    if (c==element) return true;
  return false;
}

}}

#endif // HELPERS_H
