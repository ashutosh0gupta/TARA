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
 */


#ifndef TARA_INPUT_EXCEPTION_H
#define TARA_INPUT_EXCEPTION_H

#include <stdexcept>

namespace tara {
namespace input{
class input_exception : public std::runtime_error
{
public:
  input_exception(const char* what) : runtime_error(what) {};
  input_exception(const std::string& what) : runtime_error(what) {};
};
}}

#endif // TARA_INPUT_EXCEPTION_H
