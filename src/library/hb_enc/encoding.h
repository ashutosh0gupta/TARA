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

namespace tara {
namespace hb_enc {
  class integer;
  
struct location {
private:
  z3::expr expr; // ensure this one is not visible from the outside
  uint16_t _serial;
  
  friend class hb_enc::integer;
public:
  location(location& ) = delete;
  location& operator=(location&) = delete;
  location(z3::context& ctx, std::string name, bool special = false);
  
  std::string name;
  /**
   * @brief True if this is a special location
   * 
   * This means the location is not actualy in the program, but the start symbol and the like
   */
  bool special;
  int thread; // number of the thread of this location
  int instr_no; // number of this instruction
//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------
  bool is_read;
  unsigned last_read_intruction; // for tso
//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
  /**
   * @brief The previous location in the same thread
   * 
   */
  std::weak_ptr<hb_enc::location const> prev;
  /**
   * @brief The next location in the same thread
   * 
   */
  std::weak_ptr<hb_enc::location const> next;
  
  
  bool operator==(const location &other) const;
  bool operator!=(const location &other) const;
  operator z3::expr () const;
  uint16_t serial() const;
  
  friend std::ostream& operator<< (std::ostream& stream, const location& loc);
  friend std::ostream& operator<< (std::ostream& stream, const std::shared_ptr<hb_enc::location const>& loc);
  
  void debug_print(std::ostream& stream );
};

typedef std::shared_ptr<hb_enc::location const> location_ptr;

std::string to_string (const tara::hb_enc::location& loc);
std::string get_var_from_location (location_ptr loc);
std::string operator+ (const std::string& lhs, const location_ptr& rhs);
typedef std::pair<location_ptr,location_ptr> location_pair;
std::ostream& operator<< (std::ostream& stream, const location_pair& loc_pair);


struct hb {
public:
  location_ptr loc1;
  location_ptr loc2;
  operator z3::expr () const;
  hb(location_ptr loc1, location_ptr loc2, z3::expr expr);
  uint32_t signature(); // a unique integer indentifying the hb
  
  bool operator==(const hb &other) const;
  bool operator!=(const hb &other) const;
  
  friend std::ostream& operator<< (std::ostream& stream, const hb& hb);
  void debug_print(std::ostream& stream );
  
  hb negate() const;
private:
  z3::expr expr;
  uint32_t _signature = 0;
};

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
  encoding();
  virtual ~encoding();
  
  virtual void make_locations(std::vector<std::shared_ptr<hb_enc::location>> locations) = 0;
  virtual location_ptr get_location(const std::string& name) const; // make one location from a name
  virtual hb make_hb(location_ptr loc1, location_ptr loc2) const = 0;
  virtual as make_as(location_ptr loc1, location_ptr loc2) const = 0;
  virtual bool eval_hb(const z3::model& model, location_ptr loc1, location_ptr loc2) const = 0;
  virtual std::unique_ptr<hb> get_hb(const z3::expr& hb, bool allow_equal = false) const = 0;
  virtual std::vector<hb_enc::location_ptr> get_trace(const z3::model& m) const = 0;
protected:
  std::unordered_map<std::string, location_ptr> location_map;
  void save_locations(const std::vector<std::shared_ptr<hb_enc::location>>& locations);
};
}}

#endif // TARA_API_ENCODING_H
