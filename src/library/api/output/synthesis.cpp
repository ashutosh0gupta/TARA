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

#include "synthesis.h"
#include "api/output/output_base_utilities.h"
#include <chrono>

using namespace tara;
using namespace tara::api::output;
using namespace tara::helpers;
using namespace std;

lock::lock(string name) : name(name)
{ }

barrier::barrier(string name) : name(name)
{ }

namespace tara {
namespace api {
namespace output {
ostream& operator<<(ostream& stream, const lock& lock)
{
  stream << lock.name << "(";
  for(auto l = lock.locations.begin(); l!=lock.locations.end(); l++) {
    stream << *l;
    if (!last_element(l, lock.locations)) stream << ", ";
  }
  stream << ")";
  /*stream << "[";
  for(auto v = lock.variables.begin(); v!=lock.variables.end(); v++) {
    stream << v->name;
    if (!last_element(v, lock.variables)) stream << ", ";
  }
  stream << "]";*/
  return stream;
}

ostream& operator<<(ostream& stream, const barrier& barrier)
{
  stream << barrier.name << "(";
  for(auto l = barrier.locations.begin(); l!=barrier.locations.end(); l++) {
    stream << *l ;
    if (!last_element(l, barrier.locations)) stream << ", ";
  }
  stream << ")";
  return stream;
}
}}}

synthesis::synthesis(helpers::z3interf& _z3, bool verify, bool print_nfs):
  output_base( _z3 ),
  print_nfs(print_nfs), normal_form(_z3, false, false, false, true, verify)
{}

void synthesis::init(const hb_enc::encoding& hb_encoding, const z3::solver& sol_desired, const z3::solver& sol_undesired, std::shared_ptr< const tara::program > program)
{
    output_base::init(hb_encoding, sol_desired, sol_undesired, program);
    normal_form.init(hb_encoding, sol_desired, sol_undesired, program);
}

void synthesis::output(const z3::expr& output)
{
    output_base::output(output);
    normal_form.output(output);
    
    // do the synthesis
    locks.clear();
    wait_notifies.clear();
    barriers.clear();
    lock_counter = 1;
    barrier_counter = 1;
    nf::result_type cnf = normal_form.get_result(false, false);
    
    // measure time
    auto start_time = chrono::steady_clock::now();
    synthesise(cnf);
    merge_locks();
    merge_barriers();
    merge_locks_multithread();
    merge_barriers_multithread();
    info = to_string(locks.size()) + " " + to_string(barriers.size()) + " " + to_string(wait_notifies.size()); 
    auto end_time = chrono::steady_clock::now();
    time = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
}

void synthesis::print(ostream& stream, bool machine_readable) const
{
  if (print_nfs && !machine_readable)
    normal_form.print(stream, false);
  
  // print locks and wait-notifies
  if (!machine_readable) stream << "Locks: ";
  for (auto l:locks) {
    stream << l << " ";
  }
  stream << endl;
  if (!machine_readable) stream << "Barriers: ";
  for (auto b:barriers) {
    stream << b << " ";
  }
  stream << endl;
  if (!machine_readable) stream << "Wait-notifies: ";
  for (auto hb:wait_notifies) {
    stream << hb << " ";
  }
  stream << endl;
}

void synthesis::synthesise(nf::result_type& cnf)
{
  for (auto i = cnf.begin(); i!=cnf.end(); i = cnf.erase(i)) {
    auto& disjunct = *i;
    if (!(find_lock(disjunct) || find_barrier(disjunct, i, cnf))) {
      assert (disjunct.size() > 0);
      wait_notifies.push_back(disjunct.front());
    }
  }
}

bool synthesis::find_lock(const nf::row_type& disjunct)
{
  bool found = false;
  if (disjunct.size()>=2) {
    // find all combinations
    for (auto a = disjunct.begin(); a!=disjunct.end() && !found; a++)
    {
      auto b = a;
      for (b++; b!=disjunct.end() && !found; b++) {
        // check if this is a lock
        hb_enc::tstamp_ptr loc1a = a->loc1;
        hb_enc::tstamp_ptr loc1b = b->loc2;
        hb_enc::tstamp_ptr loc2b = a->loc2;
        hb_enc::tstamp_ptr loc2a = b->loc1;
        if (loc1a->thread == loc1b->thread && loc2a->thread==loc2b->thread && 
          loc1a->instr_no >= loc1b->instr_no && loc2a->instr_no >= loc2b->instr_no
        ) {
          lock l("l" + std::to_string(lock_counter++));
          // order the locations by thread number
          if (loc1a->thread > loc2a->thread) {
            l.locations.push_back(make_pair(loc1b,loc1a));
            l.locations.push_back(make_pair(loc2b,loc2a));
          } else {
            l.locations.push_back(make_pair(loc2b,loc2a));
            l.locations.push_back(make_pair(loc1b,loc1a));
          }
          cssa::variable_set vars1 = set_intersection(lookup(loc1a).variables_orig(), lookup(loc1b).variables_orig());
          cssa::variable_set vars2 = set_intersection(lookup(loc2a).variables_orig(), lookup(loc2b).variables_orig());
          l.variables = set_union(vars1, vars2);
          locks.push_back(l);
          found = true;
        }
      }
    }
  }
  return found;
}


bool synthesis::find_barrier(const nf::row_type& disjunct, list< nf::row_type >::iterator current, nf::result_type& list)
{
  /*
   * Search if any of the disjuncts has a corresponding item in any of the other disjunctions
   */
  bool found = false;
  for (auto d : disjunct) {
    barrier b("b" + std::to_string(barrier_counter));
    b.locations.insert(d.loc2);
    // find crossing arrow
    hb_enc::tstamp_ptr start = d.loc1;
    hb_enc::tstamp_ptr end = d.loc2;
    auto a = current;
    for (a++; a!=list.end();)
    {
      bool lfound = false;
      for (auto d2 : *a) {
        if ((d2.loc1->thread == end->thread && d2.loc2->thread == start->thread &&
          d2.loc1->instr_no+1 == end->instr_no && d2.loc2->instr_no-1 == start->instr_no 
        )) {
          found = true;
          lfound = true;
          b.locations.insert(d2.loc2);
          break;
        } 
      }
      if (lfound) {
        a = list.erase(a);
      } else {
        a++;
      }
    }
    if (found) {
      barrier_counter++;
      barriers.push_back(b);
    }
  }
  return found;
}


void synthesis::merge_locks()
{
  /*
   * We merge if locks a and b are either 
   * - contained in each other
   * - would cause a deadlock
   * - are directly adjacent
   */
  for (auto i = locks.begin(); i!=locks.end(); i++) {
    assert (i->locations.size() == 2);
    // check if another lock should be merged with this one
    hb_enc::tstamp_pair& locp1a = i->locations.front();
    hb_enc::tstamp_pair& locp1b = i->locations.back();
    auto j = i;
    for (j++; j!=locks.end(); ) {
      assert (j->locations.size() == 2);
      hb_enc::tstamp_pair& locp2a = j->locations.front();
      hb_enc::tstamp_pair& locp2b = j->locations.back();
      bool found = false;
      // make sure they talk about the same threads
      if (locp1a.first->thread == locp2a.second->thread && locp1b.first->thread == locp2b.second->thread) {
        // check if one is contained in the other
        if (check_contained(locp1a, locp2a) && check_contained(locp1b, locp2b)) {
          merge_overlap(locp1a, locp2a);
          merge_overlap(locp1b, locp2b);
          found = true;
        } else if (check_overlap(locp1a, locp2a) && check_overlap(locp1b, locp2b) &&
          ((locp1a.first->instr_no > locp2a.first->instr_no) != (locp1b.first->instr_no > locp2b.first->instr_no) )
        ) {
          // check for deadlock (they overlap and one is aquired before the other
          merge_overlap(locp1a, locp2a);
          merge_overlap(locp1b, locp2b);
          found = true;
        }
      }
      if (found) {
        // add locations that were not previously merged (these were already deleted in locks[j].locations
        j = locks.erase(j);
      }
      else
        j++;
    }
  }
}

void synthesis::merge_locks_multithread()
{
  /*
   * We merge if a set of locks talks about the same set of locations. Initially every lock has two locations
   */
  for (auto i = locks.begin(); i!=locks.end(); i++) {
    // check if another lock should be merged with this one
    auto j = i;
    for (j++; j!=locks.end(); j++) {
      // the thing we want to merge with should have only two locations
      assert (j->locations.size() == 2);
      hb_enc::tstamp_pair& locp2a = j->locations.front();
      hb_enc::tstamp_pair& locp2b = j->locations.back();
      // check if one of the locations in the lock j is in the list of locations of i
      hb_enc::tstamp_pair* present = nullptr; // the location thas 
      hb_enc::tstamp_pair* other = nullptr;
      if (list_contains(i->locations, locp2a)) {
        present = &locp2a;
        other = &locp2b;
      }
      if (list_contains(i->locations, locp2b)) {
        present = &locp2b;
        other = &locp2a;
      }
      if (present != nullptr) {
        vector<list<lock>::iterator> to_delete;
        to_delete.push_back(j);
        // this is a merge candidate if we find the other corresponding items
        // the other items are those corresponding to the non-present location
        for (auto loc : i->locations) {
          if (loc != *present) { // this lock we already found
            auto h = i;
            for (h++; h!=locks.end(); h++) {
              if (h!=j) { // we don't want to test the same lock again
                if ((h->locations.front() == *other && h->locations.back() == loc) || (h->locations.back() == *other && h->locations.front() == loc)) {
                  to_delete.push_back(h);
                  break;
                }
              }
            }
          }
        }
      
        if (to_delete.size() == i->locations.size()) {
          i->locations.push_back(*other);
          for (auto it : to_delete)
            locks.erase(it);
          break;
        }
      }
    }
  }
}

void synthesis::merge_barriers()
{
  // nothing to do here
}

void synthesis::merge_barriers_multithread()
{
  for (auto i = barriers.begin(); i!=barriers.end(); i++) {
    // check if another barrier should be merged with this one
    auto j = i;
    for (j++; j!=barriers.end(); ) {
      // check if the barriers overlap
      if (intersection_nonempty(i->locations, j->locations)) {
        i->locations = set_union(i->locations, j->locations);
        j=barriers.erase(j);
      }
      else
        j++;
    }
  }
}

void synthesis::gather_statistics(api::metric& metric) const
{
  metric.additional_time = time;
  metric.additional_info = info;
}
