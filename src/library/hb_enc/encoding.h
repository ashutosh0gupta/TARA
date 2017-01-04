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

#ifndef TARA_API_ENCODING_H
#define TARA_API_ENCODING_H

#include <vector>
#include <memory>
#include <z3++.h>
#include <unordered_map>
#include <set>
#include "../helpers/helpers.h"
#include "helpers/z3interf.h"

namespace tara {
namespace hb_enc {
  class integer;

  class symbolic_event;
  class location;
  class hb;

  typedef std::shared_ptr<symbolic_event> se_ptr;
  typedef std::set<se_ptr> se_set;
  typedef std::shared_ptr<hb_enc::hb> hb_ptr;
  typedef std::vector<hb_ptr> hb_vec;
  typedef std::shared_ptr<hb_enc::location const> location_ptr;

std::string to_string (const tara::hb_enc::location& loc);
std::string get_var_from_location (location_ptr loc);
std::string operator+ (const std::string& lhs, const location_ptr& rhs);
typedef std::pair<location_ptr,location_ptr> location_pair;
std::ostream& operator<< (std::ostream& stream, const location_pair& loc_pair);



struct as {
public:
  location_ptr loc1;
  location_ptr loc2;
  operator z3::expr () const;
  as(location_ptr loc1, location_ptr loc2, z3::expr expr);
  uint32_t signature(); // a unique integer indentifying the as
  
  bool operator==(const as &other) const;
  bool operator!=(const as &other) const;
  
  friend std::ostream& operator<< (std::ostream& stream, const as& hb);
private:
  z3::expr expr;
  uint32_t _signature = 0;
};

class encoding
{
public:
  encoding(helpers::z3interf& z3);
  // encoding(z3::context& ctx);
  virtual ~encoding();
  
  virtual void record_event( se_ptr& ) = 0;
  void record_rf_map( std::set< std::tuple<std::string, hb_enc::se_ptr, hb_enc::se_ptr> >& );

  virtual void make_location( std::shared_ptr<hb_enc::location> locations ) = 0;
  virtual void make_locations(std::vector<std::shared_ptr<hb_enc::location>> locations) = 0;
  virtual location_ptr get_location(const std::string& name) const; // make one location from a name
  virtual hb make_hb(location_ptr loc1, location_ptr loc2) const = 0;
  virtual hb make_hb_po(location_ptr loc1, location_ptr loc2) const = 0;
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  hb mk_hb_c11_hb(const se_ptr& before, const se_ptr& after);
  hb mk_hb_c11_sc(const se_ptr& before, const se_ptr& after);
  hb mk_hb_c11_mo(const se_ptr& before, const se_ptr& after);
  hb mk_hb_thin(const se_ptr&, const se_ptr& );
  hb mk_hbs(const se_ptr& loc1, const se_ptr& loc2);
  z3::expr mk_hbs(const se_set& loc1, const se_ptr& loc2);
  z3::expr mk_hbs(const se_ptr& loc1, const se_set& loc2);
  z3::expr mk_hbs(const se_set& loc1, const se_set& loc2);
  z3::expr mk_ghbs( const se_ptr& before, const se_ptr& after );
  z3::expr mk_ghbs( const se_ptr& before, const se_set& after );
  z3::expr mk_ghbs( const se_set& before, const se_ptr& after );
  z3::expr mk_ghb_thin( const se_ptr& before, const se_ptr& after );

  //calls related c11 model
  z3::expr mk_ghbs_c11_hb( const se_set& before, const se_ptr& after );
  z3::expr mk_ghb_c11_hb( const se_ptr& before, const se_ptr& after );
  z3::expr mk_ghb_c11_mo( const se_ptr& before, const se_ptr& after );
  z3::expr mk_ghb_c11_sc( const se_ptr& before, const se_ptr& after );


  z3::expr mk_guarded_forbid_expr( const hb_vec& );
  z3::expr mk_expr( const hb_vec& );

  bool eval_hb( const z3::model& m, const se_ptr& before, const se_ptr& after ) const;

// protected:
  std::set< std::tuple<std::string, hb_enc::se_ptr, hb_enc::se_ptr> > rf_map;
  std::map< std::string, hb_enc::hb_ptr > current_rf_map;
public:
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
  virtual as make_as(location_ptr loc1, location_ptr loc2) const = 0;
  virtual bool eval_hb(const z3::model& model, location_ptr loc1, location_ptr loc2) const = 0;
  virtual hb_ptr get_hb(const z3::expr& hb, bool allow_equal = false) const = 0;
  virtual std::vector<hb_enc::location_ptr> get_trace(const z3::model& m) const = 0;
  // std::list<z3::expr> get_hbs( z3::model& m ) const;
  std::vector<hb_ptr> get_hbs( z3::model& m );
  helpers::z3interf& z3;
  // z3::context& ctx;
protected:
  std::unordered_map<std::string, location_ptr> location_map;
  void save_locations(const std::vector<std::shared_ptr<hb_enc::location>>& locations);
};
}}

#endif // TARA_API_ENCODING_H
