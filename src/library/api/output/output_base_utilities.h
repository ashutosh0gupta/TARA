#include "output_base.h"
#include "helpers/helpers.h"

namespace tara {
namespace api {
namespace output {
  
/***************************
 * Useful utility functions
 ***************************/

const tara::instruction& output_base::lookup(const hb_enc::location_ptr& loc) const {
  return program->lookup_location(loc); 
}

cssa::variable_set output_base::read_write(const hb_enc::hb& hb) const
{
  return helpers::set_intersection(lookup(hb.loc1).variables_read_orig, lookup(hb.loc2).variables_write_orig);
}

cssa::variable_set output_base::access_access(const hb_enc::hb& hb) const
{
  return helpers::set_intersection(lookup(hb.loc1).variables_orig(), lookup(hb.loc2).variables_orig());
}

cssa::variable_set output_base::write_read(const hb_enc::hb& hb) const
{
  return helpers::set_intersection(lookup(hb.loc1).variables_write_orig, lookup(hb.loc2).variables_read_orig);
}

cssa::variable_set output_base::write_write(const hb_enc::hb& hb) const
{
  return helpers::set_intersection(lookup(hb.loc1).variables_write_orig, lookup(hb.loc2).variables_write_orig);
}

cssa::variable_set output_base::read_read(const hb_enc::hb& hb) const
{
  return helpers::set_intersection(lookup(hb.loc1).variables_read_orig, lookup(hb.loc2).variables_read_orig);
}

bool output_base::same_inverted(const hb_enc::hb& hb1, const hb_enc::hb& hb2) const
{
  return (hb1.loc1==hb2.loc2 && hb1.loc2==hb2.loc1);
}

bool output_base::order_hb(hb_enc::hb& hb1, hb_enc::hb& hb2) const
{
  if (hb2.loc2 == hb1.loc1) {
    hb_enc::hb temp = hb1;
    hb1 = hb2;
    hb2 = temp;
    return true;
  }
  return (hb1.loc2 == hb2.loc1);
}


void output_base::order_locations(hb_enc::location_ptr& loc1, hb_enc::location_ptr& loc2) {
  assert (loc1->thread==loc2->thread);
  if (loc1->instr_no > loc2->instr_no) {
    hb_enc::location_ptr helper = loc1;
    loc1 = loc2;
    loc2 = helper;
  }
}

void output_base::order_locations_thread(hb_enc::location_ptr& loc1, hb_enc::location_ptr& loc2) {
  if (loc1->thread > loc2->thread) {
    hb_enc::location_ptr helper = loc1;
    loc1 = loc2;
    loc2 = helper;
  }
}

bool output_base::check_overlap(const hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2)
{
  if (locs1.first->thread == locs2.first->thread) {
    if ((locs1.first->instr_no <= locs2.first->instr_no && locs2.first->instr_no <= locs1.second->instr_no) ||
      (locs1.first->instr_no <= locs2.first->instr_no && locs1.first->instr_no <= locs2.second->instr_no)
    )
      return true;
  }
  return false;
}

bool output_base::check_contained(const hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2)
{
  if (locs1.first->thread == locs2.first->thread) {
    if ((locs1.first->instr_no <= locs2.first->instr_no && locs2.second->instr_no <= locs1.second->instr_no) ||
      (locs2.first->instr_no <= locs1.first->instr_no && locs1.second->instr_no <= locs2.second->instr_no)
    )
      return true;
  }
  return false;
}

void output_base::merge_overlap(hb_enc::tstamp_pair& locs1, const hb_enc::tstamp_pair& locs2)
{
  locs1.first=locs1.first->instr_no <= locs2.first->instr_no ? locs1.first : locs2.first;
  locs1.second=locs1.second->instr_no <= locs2.second->instr_no ? locs1.second : locs2.second;
}

}}}
