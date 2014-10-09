#include "cssa/program.h"
#include "helpers/helpers.h"

using namespace tara;
using namespace tara::helpers;
using namespace tara::cssa;

using namespace std;

void program::print_hb(const z3::model& m, ostream& out, bool machine_readable) const
{ 
  if (!machine_readable){
    // initial values
    auto git = initial_variables.begin();
    if (git!=initial_variables.end()) {
      out << "init: [ ";
      bool first = true;
      for(; git!=initial_variables.end(); git++) {
        if (!first) {
          out << "; ";
        }
        first = false;
        out << (string)*git << "=" << m.eval(*git);
      }
      out << " ]" << endl;
    }
  }
  
  // get into right order
  vector<instruction> ordered;
  for (unsigned t=0; t<threads.size(); t++) {
    const thread& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size(); i++) {
      std::vector<instruction>::iterator it;
      for (it = ordered.begin() ; it != ordered.end(); ++it) {
        if (!_hb_encoding.eval_hb(m, it->loc, thread[i].loc)) {
          // insert here when the new instruction is not after *it
          break;
        }
      }
      ordered.insert(it, thread[i]);
      // terminate if assumption fails
      if (thread[i].type==instruction_type::assume && !m.eval(thread[i].instr).get_bool())
        break;
    }
  }
  
  // output with variables
  for (instruction i: ordered) {
    if (!machine_readable) {
      out << i;
      auto vit = i.variables_write.begin();
      if (vit!=i.variables_write.end()) {
        out << " [ ";
        bool first = true;
        for(; vit!=i.variables_write.end(); vit++) {
          if (!first) {
            out << "; ";
          }
          first = false;
          out << get_before_hash(*vit) << ".=" << m.eval(*vit);
        }
        out << " ]";
      }
      // if this is a failing assumption
      if (i.type==instruction_type::assume || i.type==instruction_type::assert)
        if (!m.eval(i.instr).get_bool())
          out << " (fails)";
      out << endl;
    } else {
      out << i.loc << " ";
    }
  }
  out << endl;
}

list<z3::expr> program::get_hbs(z3::model& m) const
{
  list<z3::expr> result;
  z3::expr_vector asserted = _z3.c.collect_last_asserted_linear_constr();
  
  for(unsigned i = 0; i<asserted.size(); i++) {
    unique_ptr<hb_enc::hb> hb = _hb_encoding.get_hb(asserted[i]);
    if (hb && !hb->loc1->special && !hb->loc2->special && hb->loc1->thread != hb->loc2->thread) {
      //z3::expr hb2 = _hb_encoding.make_hb(hb->loc1, hb->loc2);
      //cout << asserted[i] << " | " << (z3::expr)*hb << " | " << *hb << " | " << (z3::expr)hb2 << endl;
      //assert(m.eval(*hb).get_bool() == m.eval(hb2).get_bool());
      assert(_hb_encoding.eval_hb(m, hb->loc1, hb->loc2));
      result.push_back(asserted[i]);
    }
  }
  
  return result;
}

void program::print_dot(ostream& stream, vector< hb_enc::hb >& hbs) const
{
  // output the program as a dot file
  stream << "digraph hbs {" << endl;
  // output labels
  for (unsigned t=0; t<threads.size(); t++) {
    const thread& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size(); i++) {
      stream << thread[i].loc->name << "[label=\"" << thread[i] << "\"]" << endl;
    }
  }
  // add white edges between instructions
  for (unsigned t=0; t<threads.size(); t++) {
    const thread& thread = *threads[t];
    for (unsigned i=0; i<thread.instructions.size()-1; i++) {
      stream << thread[i].loc->name << "->" << thread[i+1].loc->name << " [color=white]" << endl;
    }
  }
  for (hb_enc::hb hb : hbs) {
    stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << endl;
  }
  stream << "}" << endl;
}

z3::expr program::get_initial(const z3::model& m) const
{
  z3::expr res = _z3.c.bool_val(true);
  for (variable v:initial_variables) {
    z3::expr vname = v;
    z3::expr e = m.eval(vname);
    if (((Z3_ast)vname) != ((Z3_ast)e))
      res = res && vname == e;
  }
  return res;
}