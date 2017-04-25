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
  class tstamp;
  class hb;

  typedef std::shared_ptr<symbolic_event> se_ptr;
  typedef std::set<se_ptr> se_set;
  typedef std::shared_ptr<hb_enc::hb> hb_ptr;
  typedef std::vector<hb_ptr> hb_vec;
  typedef std::shared_ptr<hb_enc::tstamp const> tstamp_ptr;
  typedef std::shared_ptr<hb_enc::tstamp> tstamp_var_ptr;

std::string to_string (const tara::hb_enc::tstamp& loc);

std::string operator+ (const std::string& lhs, const tstamp_ptr& rhs);
typedef std::pair<tstamp_ptr,tstamp_ptr> tstamp_pair;
std::ostream& operator<< (std::ostream& stream, const tstamp_pair& loc_pair);



struct as {
public:
  tstamp_ptr loc1;
  tstamp_ptr loc2;
  operator z3::expr () const;
  as(tstamp_ptr loc1, tstamp_ptr loc2, z3::expr expr);
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

  virtual void make_tstamp( tstamp_var_ptr ) = 0;
  virtual void add_time_stamps(std::vector< tstamp_var_ptr > ) = 0;
  virtual tstamp_ptr get_tstamp(const std::string& name) const; // make one tstamp from a name
  virtual hb make_hb(tstamp_ptr loc1, tstamp_ptr loc2) const = 0;
  virtual hb make_hb_po(tstamp_ptr loc1, tstamp_ptr loc2) const = 0;
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
  z3::expr get_rf_bit( const se_ptr&, const se_ptr& ) const;
  z3::expr get_rf_bit( const tara::variable&, const se_ptr&, const se_ptr& ) const;
public:
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
  virtual as make_as(tstamp_ptr loc1, tstamp_ptr loc2) const = 0;
  virtual bool eval_hb(const z3::model& model, tstamp_ptr loc1, tstamp_ptr loc2) const = 0;
  virtual hb_ptr get_hb(const z3::expr& hb, bool allow_equal = false) const = 0;
  virtual std::vector<hb_enc::tstamp_ptr> get_trace(const z3::model& m) const = 0;
  // std::list<z3::expr> get_hbs( z3::model& m ) const;
  std::vector<hb_ptr> get_hbs( z3::model& m );
  helpers::z3interf& z3;
  // z3::context& ctx;
protected:
  std::unordered_map<std::string, tstamp_ptr> tstamp_map;
  void save_tstamps(const std::vector<hb_enc::tstamp_var_ptr>& );
};
}}

#endif // TARA_API_ENCODING_H
