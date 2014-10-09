/*
  Added by Ashutosh Gupta
*/
#include"smt_context.h"
#include"smt_model_generator.h"
#include"proto_model.h"
#include"for_each_expr.h"
#include"ast_ll_pp.h"
#include"ast_pp.h"
#include"ast_smt2_pp.h"
#include "theory_arith.h"

namespace smt {



  app* model_generator::search_matching_term( func_decl* f, 
                                              func_entry const* entry ) {
    enode_vector::const_iterator it  = m_context->begin_enodes_of( f );
    enode_vector::const_iterator end = m_context->end_enodes_of  ( f );
    unsigned arity = f->get_arity();
   
    for(; it != end; it++ ) {
      enode* e = *it;
      app* fargs = e->get_owner();
      unsigned j = 0;
      for(; j < arity; j++ ) {
        expr* arg = fargs->get_arg(j);
        enode * arg_en= m_context->get_enode( arg );
        app * val = 0;
        m_root2value.find( arg_en->get_root(), val );
        if( val == 0 ) {
          if( arg != entry->get_arg(j) ) break;
          UNREACHABLE();
        }else{
          if( val != entry->get_arg(j) ) break;
        }
      }
      if( j == arity ) {
        // ERROR CHECK get_value( e->get_root(), val ) == entry->get_result()
        // do we want fargs to be global??? - breaks down the finiteness 
        // argument
        return fargs;
        break;
      }
    }
    //ERROR;
    return 0;
  }


  void model_generator::collect_asserted_linear_constr( ast_ref_vector& atoms ) {
    // find arithmatic theory
    family_id arith_id = m_manager.get_family_id("arith");    
    theory * th = m_context->get_theory( arith_id );
    theory_mi_arith* ar_th = reinterpret_cast<theory_mi_arith*>(th);
    // collect the asserted atoms
    vector< std::pair<bool_var, bool> > bvars; 
    ar_th->collect_asserted_atoms( bvars );
    for( unsigned i=0; i < bvars.size(); i++ ) {
      bool_var bv = bvars[i].first;
      bool  is_tr = bvars[i].second;
      expr* e = m_context->bool_var2expr( bv );
      if( !is_tr ) {
        e = m_manager.mk_not( e );
      }
      atoms.push_back( e );
    }
    // filter out non intersting atoms
    
    // return

  }
  
};
