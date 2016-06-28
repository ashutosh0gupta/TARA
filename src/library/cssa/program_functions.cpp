#include <iostream>
#include <fstream>
#include "cssa/program.h"
#include "helpers/helpers.h"

using namespace tara;
using namespace tara::helpers;
using namespace tara::cssa;

using namespace std;

void program::print_hb(const z3::model& m, ostream& out, bool machine_readable) const
{  if( is_mm_declared() ) { wmm_print_dot( m ); return; } //wmm support
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
    z3::expr atom = asserted[i];
    unique_ptr<hb_enc::hb> hb = _hb_encoding.get_hb( atom );
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

//--------------------------------------------------------------------------
//start of wmm support
//--------------------------------------------------------------------------

void program::wmm_print_dot( z3::model m ) const {
  std::ofstream myfile;
  myfile.open( "/tmp/.iteration-sat-dump" );
  if( myfile.is_open() ) {
    wmm_print_dot( myfile, m );
  }else{
    // warning( "failed to open /tmp/.iteration-sat-dump");
  }
  myfile.close();
}

void program::wmm_print_dot( ostream& stream, z3::model m ) const {
    // output the program as a dot file
  stream << "digraph hbs {" << endl;
  // output labels

  for (unsigned t=0; t<threads.size(); t++) {
    const thread& thread = *threads[t];
    stream << "subgraph cluster_" << t << " {\n";
    stream << "color=lightgrey;\n";
    stream << "label = \"" << thread.name << "\"";
    for (unsigned i=0; i < thread.instructions.size(); i++) {
      stream << "\"" << thread[i].loc->name << "\""
             << " [label=\"" << thread[i] << "\"";
      z3::expr v = m.eval( thread[i].path_constraint );
      if( Z3_get_bool_value( v.ctx(), v)  != Z3_L_TRUE) {
        stream << ",color=gray";
      }
      stream << "]"<< endl;
    }
    stream << " }\n";
  }

  stream << "pre" << " [label=\""  << phi_pre << "\"]" << endl;
  stream << "post" << " [label=\"" << phi_post << "\"]" << endl;

  // add white edges between instructions
  for (unsigned t=0; t<threads.size(); t++) {
    const thread& thread = *threads[t];
    if( thread.instructions.size() > 0 ) {
      stream << "pre ->" << "\"" << thread[0].loc->name << "\""
             << " [color=white]" << endl;
    }
    for (unsigned i=0; i<thread.instructions.size()-1; i++) {
      stream << "\"" << thread[i].loc->name << "\""  << "->"
             << "\"" << thread[i+1].loc->name << "\""
             << " [color=white]" << endl;
    }
    if( thread.instructions.size() > 0 ) {
      stream << "\"" << thread[thread.instructions.size()-1].loc->name << "\""
             << "-> post [color=white]" << endl;
    }
  }

  for( auto& it : reading_map ) {
    std::string bname = std::get<0>(it);
    z3::expr b = _z3.c.bool_const(  bname.c_str() );
    z3::expr bv = m.eval( b );
    if( Z3_get_bool_value( bv.ctx(), bv) == Z3_L_TRUE ) {
      hb_enc::se_ptr wr = std::get<1>(it);
      hb_enc::se_ptr rd = std::get<2>(it);
      unsigned wr_tid      = wr->e_v->thread;
      std::string wr_name;
      if( wr_tid == threads.size() ) {
        wr_name = "pre";
      }else{
        const thread& wr_thread = *threads[wr_tid];
        unsigned wr_instr_no = wr->e_v->instr_no;
        wr_name = wr_thread[wr_instr_no].loc->name;
      }
      unsigned rd_tid      = rd->e_v->thread;
      std::string rd_name;
      if( rd_tid == threads.size() ) {
        rd_name = "post";
      }else{
        const thread& rd_thread = *threads[rd_tid];
        unsigned rd_instr_no = rd->e_v->instr_no;
        rd_name = rd_thread[rd_instr_no].loc->name;
      }
      stream << "\"" << wr_name  << "\" -> \"" << rd_name << "\""
             << "[color=blue]"<< endl;
      //fr
      std::set< hb_enc::se_ptr > fr_wrs;
      auto it = wr_events.find(rd->prog_v);
      if( it != wr_events.end() ) { // fails for dummy variable
        for( const auto& after_wr : it->second ) {
          z3::expr v = m.eval( after_wr->guard );
          if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE) {
            if( hb_eval( m, wr, after_wr ) ) {
              unsigned after_wr_tid      = after_wr->e_v->thread;
              const thread& after_wr_thread = *threads[after_wr_tid];
              unsigned after_wr_instr_no = after_wr->e_v->instr_no;
              std::string after_wr_name =
                after_wr_thread[after_wr_instr_no].loc->name;
              stream << "\"" << rd_name  << "\" -> \"" << after_wr_name << "\""
                     << "[color=orange,constraint=false]"<< endl;
            }
          }
        }
      }
    }
  }

  for(const variable& v1 : globals ) {
    std::set< hb_enc::se_ptr > wrs;
    auto it = wr_events.find( v1 );
    for( const auto& wr : it->second ) {
      z3::expr v = m.eval( wr->guard );
      if( Z3_get_bool_value( v.ctx(), v)  == Z3_L_TRUE)
        wrs.insert( wr );
    }
    hb_enc::se_vec ord_wrs;
    while( !wrs.empty() ) {
      hb_enc::se_ptr min;
      for ( auto wr : wrs ) {
        if( min ) {
          if( hb_eval( m, wr, min ) ) {
            min = wr;
          }
        }else{
          min = wr;
        }
      }
      ord_wrs.push_back(min);
      wrs.erase( min );
    }

    for( unsigned i = 1; i < ord_wrs.size(); i++  ) {
      unsigned wr_tid      = ord_wrs[i-1]->e_v->thread;
      std::string wr_name;
      if( wr_tid == threads.size() ) {
        wr_name = "pre";
      }else{
        const thread& wr_thread = *threads[wr_tid];
        unsigned wr_instr_no = ord_wrs[i-1]->e_v->instr_no;
        wr_name = wr_thread[wr_instr_no].loc->name;
      }

      unsigned wr1_tid = ord_wrs[i]->e_v->thread;
      unsigned wr1_instr_no = ord_wrs[i]->e_v->instr_no;
      assert( wr1_tid != threads.size() );
      const thread& wr1_thread = *threads[wr1_tid];
      std::string wr1_name = wr1_thread[wr1_instr_no].loc->name;

      stream << "\"" << wr_name << "\" -> \"" << wr1_name << "\""
             << "[color=green]" <<  endl;
    }
  }

  // for (hb_enc::hb hb : hbs) {
  //   stream << hb.loc1->name << "->" << hb.loc2->name << " [constraint = false]" << endl;
  // }
  stream << "}" << endl;

}

//--------------------------------------------------------------------------
//end of wmm support
//--------------------------------------------------------------------------
