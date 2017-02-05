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
 */

#ifndef TARA_VARIABLE_H
#define TARA_VARIABLE_H

#include "helpers/helpers.h"
#include "helpers/z3interf.h"
// #include "api/options.h"
// #include "hb_enc/symbolic_event.h"
// #include "api/metric.h"

namespace tara {
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


  inline tara::variable get_unprimed(const tara::variable& variable) {
    //converting the primed to unprimed by removing the dot
    if (variable.name[variable.name.size()-1] != '.') {
      return variable;
    } else {
      std::string v1 = variable.name.substr(0, variable.name.size()-1);
      return tara::variable(v1, variable.sort);
    }
  }

}

#endif // TARA_VARIABLE_H
