
#include "z3interf.h"

/****************
 * Template functions
 ****************/ 

template <class value>
void tara::helpers::z3interf::min_unsat(z3::solver& sol, std::list< value >& items, std::function< z3::expr(value) > translate) {
  std::vector<z3::expr> triggers;
  // translate items to expr
  for (auto i: items) {
    z3::expr trigger = sol.ctx().fresh_constant("implied_assumption", sol.ctx().bool_sort());
    triggers.push_back(trigger);
    z3::expr translation = translate(i);
    // std::cout<<"\nexpr 1 "<<translation<<"\n";
    // std::cout<<"\nexpr 2 "<<trigger<<"\n";
    sol.add(implies(trigger,translation));
  }
  
  // for many items, first do unsat core
  if (items.size() > 6) { // 10) {
    z3::check_result r = sol.check(triggers.size(), &triggers[0]);
    if (r==z3::sat) {
      //std::cout << sol.get_model() << std::endl;
      return;
    }
    z3::expr_vector core = sol.unsat_core();
    std::unordered_set<Z3_ast> core_set;
    for (unsigned i = 0; i<core.size(); i++)
      core_set.insert(core[i]);
    auto t = triggers.begin();
    auto i = items.begin();
    for (; t != triggers.end() && i != items.end(); ) {
      auto found = core_set.find(*t);
      if (found == core_set.end()) {
        i = items.erase(i);
        t = triggers.erase(t);
      } else {
        i++;
        t++;
      }
    }
  }
  
  // test which expressions make the solver return unsat
  auto t = triggers.begin();
  auto i = items.begin();
  for (; t != triggers.end() && i != items.end(); ) {
    // assemble triggers to use
    z3::expr_vector assertions(sol.ctx());
    for (auto t1 = triggers.begin(); t1 != triggers.end(); t1++) {
      if (t1 != t) assertions.push_back(*t1);
    }
    if (sol.check(assertions) == z3::unsat) {
      i = items.erase(i);
      t = triggers.erase(t);
    } else {
      //std::cout << sol.get_model() << std::endl;
      i++;
      t++;
    }
  }
}

template <class value>
void tara::helpers::z3interf::remove_implied(const z3::expr& fixed, std::list< value >& items, std::function< z3::expr(value) > translate) {
  /* Idea:
   * We create triggers for each item (equivalence trigger, not implication)
   * Then we invert one trigger at a time and test unsat
   * If the result is unsat it means this element contradicts one of the other elements.
   * It is therefore implied by the other elements
   */
  
  z3::solver sol = z3interf::create_solver(fixed.ctx());
  sol.add(fixed);
  std::vector<z3::expr> triggers;
  // translate items to expr
  for (auto i: items) {
    z3::expr trigger = sol.ctx().fresh_constant("trigger", sol.ctx().bool_sort());
    triggers.push_back(trigger);
    z3::expr translation = translate(i);
    sol.add(trigger == translation);
  }
  
  // test which expressions make the solver return unsat
  auto i = items.begin();
  for (unsigned t = 0; t < triggers.size(); ) {
    // invert one trigger in this
    z3::expr temp = triggers[t];
    triggers[t] = !temp;
    z3::check_result r = sol.check(triggers.size(), &triggers[0]);
    triggers[t] = temp;
    if (r == z3::unsat) {
      i = items.erase(i);
      triggers.erase(triggers.begin() + t);
    } else {
      i++;
      t++;
    }
  }
}
