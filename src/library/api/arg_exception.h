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

#ifndef TARA_API_OUTPUT_ARG_EXCEPTION_H
#define TARA_API_OUTPUT_ARG_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace tara {
namespace api {

class arg_exception : public std::runtime_error
{
public:
  arg_exception(const char* what) : runtime_error(what){}
  arg_exception(const std::string& what) : runtime_error(what){}
};
}
}


#endif // TARA_API_OUTPUT_ARG_EXCEPTION_H
