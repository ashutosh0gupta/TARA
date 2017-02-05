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

#include "bugs.h"
#include "api/output/output_base_utilities.h"
#include <chrono>

using namespace tara;
using namespace tara::api::output;
using namespace tara::helpers;
using namespace std;

bug::bug(bug_type type) : type(type)
{ }

bug::bug( bug_type type, list< hb_enc::tstamp_ptr > locations) : type(type), locations(locations)
{ }

bool bug::operator==(const bug& rhs) const
{
  bool r = rhs.type == this->type && rhs.locations == this->locations; 
  return r;
}

bool bug::operator!=(const bug& rhs) const
{
  return !((*this) == rhs);
}


namespace tara {
namespace api {
namespace output {
ostream& operator<<(ostream& stream, const bug& bug)
{
  stream << "Potential ";
  switch (bug.type) {
    case bug_type::two_stage :
      stream << "two stage access";
      break;
    case bug_type::data_race :
      stream << "data race";
      break;
    case bug_type::define_use :
      stream << "define-use";
      break;
    case bug_type::atomicity_violation :
      stream << "other atomicity violation";
      break;
  }
  stream << " bug: ";
  for (auto it = bug.locations.begin(); it!=bug.locations.end(); it++) {
    stream << *it;
    if (!last_element(it, bug.locations))
      stream << " -> ";
  }
  return stream;
}
}}}

bugs::bugs(options& o_, helpers::z3interf& _z3): output_base(o_, _z3), normal_form(o_, _z3,true, false, false, false)
{
  print_nfs = helpers::exists( o.mode_options, std::string("nfs")  );
}

void bugs::init(const hb_enc::encoding& hb_encoding, const z3::solver& sol_desired, const z3::solver& sol_undesired, std::shared_ptr< const tara::program > program)
{
    output_base::init(hb_encoding, sol_desired, sol_undesired, program);
    normal_form.init(hb_encoding, sol_desired, sol_undesired, program);
}

void bugs::output(const z3::expr& output)
{
    output_base::output(output);
    normal_form.output(output);
    
    // do the analysis
    unclassified = normal_form.get_result(true, true);
    auto start_time = chrono::steady_clock::now();
    find_bugs(unclassified);
    auto end_time = chrono::steady_clock::now();
    time = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
}

void bugs::print(ostream& stream, bool machine_readable) const
{
  if (print_nfs && !machine_readable)
    normal_form.print(stream, false);
  
  // print bugs
  for (auto b: bug_list) {
    stream << b << endl;
  }
  
  if (unclassified.size() > 0) {
    stream << "Unclassified bugs" << endl;
    normal_form.print_one(stream, false, unclassified, true);
  }

  stream << endl;
}

void bugs::find_bugs(nf::result_type& dnf)
{
  
  for (auto i = dnf.begin(); i!=dnf.end(); ) {
    auto& conj = *i;
    clean_row(conj);
    if (conj.size()==0) {
      i = dnf.erase(i);
    }
    else if (race_condition(conj) || two_stage(conj, i, dnf) || 
       define_use(conj))
      i=dnf.erase(i);
    else
      i++;
  }
  remove_duplicates(bug_list);
}

void bugs::clean_row(nf::row_type& conj)
{
  for (auto i = conj.begin(); i!=conj.end();) {
    const hb_enc::hb& hb1 = *i;
    if (write_read(hb1).size()>=1 && lookup(hb1.loc2).type==instruction_type::assume) {
      i = conj.erase(i);
    } else{
      i++;
    }
  }
}


bool bugs::race_condition(nf::row_type& conj)
{  
  /* Criteria:
   * AV it is access -> access -> access of same variable
   * 
   * DR is read -> write -> read, where write does not also read the variable
   */
  for (auto i = conj.begin(); i!=conj.end(); i++) {
    auto j = i;
    for (j++; j!=conj.end(); j++) {
      hb_enc::hb hb1 = *i;
      hb_enc::hb hb2 = *j;
      if (order_hb(hb1, hb2)) {
        // check data-flow
        if (hb1.loc1->thread == hb2.loc2->thread) {
          const tara::instruction& lastloc = lookup(hb2.loc2);
          auto common_vars = set_intersection(read_write(hb1), write_write(hb2));
          if (common_vars.size()>0 && !intersection_nonempty(common_vars, lastloc.variables_read_orig)) {
            bug_list.emplace_back(bug_type::data_race, list<hb_enc::tstamp_ptr>({hb1.loc1, hb1.loc2, hb2.loc2}));
            return true;
          } else if (intersection_nonempty(access_access(hb1), access_access(hb2))) {
            bug_list.emplace_back(bug_type::atomicity_violation, list<hb_enc::tstamp_ptr>({hb1.loc1, hb1.loc2, hb2.loc2}));
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool bugs::define_use(nf::row_type& conj)
{  
  /* Criteria:
   * a hb where the first reads the variable and the second one writes it. 
   * If this read can be before any write to the variable then this is a define-use bug.
   * For define use the define must not read the variable it writes to.
   * 
   */
  for (const auto& hb : conj) {
    tara::variable_set var;
    if ((var=read_write(hb)).size()>0) {
      if (!intersection_nonempty(lookup(hb.loc2).variables_read_orig, lookup(hb.loc2).variables_write_orig) && first_assignment(*(var.begin()), conj.front().loc1, conj)) {
        bug_list.emplace_back(bug_type::define_use, list<hb_enc::tstamp_ptr>({conj.front().loc1, conj.front().loc2}));
        return true;
      }
    }
  }
  return false;
}

bool bugs::two_stage(nf::row_type& conj, nf::result_type::iterator current, nf::result_type& list)
{  
  /* Criteria:
   * two threads use different operations, each of which is atomic. However, the combination is not atomic.
   */
  for (auto i = conj.begin(); i!=conj.end(); i++) {
    auto j = i;
    for (j++; j!=conj.end(); j++) {
      hb_enc::hb hb1 = *i;
      hb_enc::hb hb2 = *j;
      if (hb1.loc1->thread == hb2.loc2->thread && hb1.loc2->thread == hb2.loc1->thread) {
        // they must not cross, and not share a location
        if ((hb1.loc1->instr_no > hb2.loc2->instr_no && hb1.loc2->instr_no > hb2.loc1->instr_no) ||
          (hb1.loc1->instr_no < hb2.loc2->instr_no && hb1.loc2->instr_no < hb2.loc1->instr_no)) {
          if (hb1.loc1->instr_no > hb2.loc2->instr_no) { // swap them to bring them in good order
            hb_enc::hb temp = hb1;
            hb1 = hb2;
            hb2 = temp;
          }
          if ((write_read(hb1).size()>0 && read_write(hb2).size()>0) ||  (write_read(hb2).size()>0 && read_write(hb1).size()>0)) {
            bug_list.emplace_back(bug_type::two_stage, std::list<hb_enc::tstamp_ptr>({hb1.loc1, hb1.loc2, hb2.loc1, hb2.loc2}));
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool bugs::first_assignment(const tara::variable& variable, hb_enc::tstamp_ptr loc, const nf::row_type& hbs)
{
  z3::expr phi_po = program->phi_po;
  z3::solver sol = z3interf::create_solver(phi_po.ctx());
  sol.add(phi_po);
  // gather locations (other than loc) and assert they come later
  z3::expr hb_other_later = phi_po.ctx().bool_val(true);
  if( program->is_original() ) {
    auto assignments = program->get_assignments_to_variable(variable);
    assert (assignments.size() > 0);
    for (shared_ptr<const tara::instruction> instr : assignments) {
      if (instr->loc != loc) {
        hb_other_later = hb_other_later && _hb_encoding->make_hb(loc, instr->loc);
      }
    }
  }else{
    bugs_error( "c++ programs are not supported yet!!" );
  }
  
  z3::expr hb_exp = phi_po.ctx().bool_val(true);
  for (const auto& h : hbs) {
    hb_exp = hb_exp && h;
  }
  
  sol.add(hb_exp);
  sol.add(hb_other_later);
  
  z3::check_result r = sol.check();
    
  return r==z3::sat;
}


void bugs::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  
  // get bug statistics
  unsigned two_stage = 0;
  unsigned data_race = 0;
  unsigned define_use = 0;
  unsigned atomicity_violation = 0;
  
  for (auto& b : bug_list) {
    switch (b.type) {
      case bug_type::two_stage :
        two_stage++;
        break;
      case bug_type::data_race :
        data_race++;
        break;
      case bug_type::define_use :
        define_use++;
        break;
      case bug_type::atomicity_violation :
        atomicity_violation++;
        break;
    }
  }
  
  unsigned unclassified_count = unclassified.size();
  
  string found_bug;
  if (two_stage + data_race + atomicity_violation + define_use > 0)
    found_bug = "found_bug";
  
  metric.additional_time = time;
  metric.additional_info = 
  to_string(two_stage) + " " + to_string(data_race) + " " + to_string(atomicity_violation) + " " + to_string(define_use) + "% & " + to_string(unclassified_count) + " " + found_bug;
}
