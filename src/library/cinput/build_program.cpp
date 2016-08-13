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


#include <string> 
#include "build_program.h"
#include "cinput_exception.h"
#include "helpers/z3interf.h"
#include <z3.h>
using namespace tara;
using namespace tara::cinput;
using namespace tara::helpers;

#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// pragam'ed to aviod warnings due to llvm included files

#include "llvm/IRReader/IRReader.h"

#include "llvm/IR/InstIterator.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/IR/DebugInfo.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"

// #include "llvm/IR/InstrTypes.h"
// #include "llvm/IR/BasicBlock.h"

#include "llvm/IRReader/IRReader.h"
// #include "llvm/Support/Debug.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Dwarf.h"

#include "llvm/Analysis/CFGPrinter.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/ADT/StringMap.h"

#pragma GCC diagnostic pop



#define UNSUPPORTED_INSTRUCTIONS( InstTYPE, Inst )                       \
  if(llvm::isa<llvm::InstTYPE>(Inst) ) {                                 \
     cinput_error( "Unsupported instruction!!");                         \
  }

  hb_enc::depends_set data_dep_ses;
  hb_enc::depends_set ctrl_dep_ses;
  build_program::local_data_dependency local_map;

//----------------------------------------------------------------------------
// code for input object generation

void split_step::print( std::ostream& os ) const {
  os << "--" << block_id << ":"<< succ_num << "\n";
  os << cond << "\n";
}

void cinput::print( std::ostream& os, const split_history&  hs ) {
  os << "[\n";
  for( auto& s : hs ) s.print( os );
  os << "]\n";
}


char build_program::ID = 0;
void
build_program::join_histories( const std::vector< llvm::BasicBlock* >& preds,
                               const std::vector< split_history >& hs,
                               split_history& h,
                               z3::expr& path_cond,
                               std::map< const llvm::BasicBlock*, z3::expr >& conds) {
  h.clear();
  if(hs.size() == 0 ) {
    split_step ss( NULL, 0, 0, z3.mk_false() );
    h.push_back(ss);
    path_cond = z3.mk_false();
    return;
  }
  if(hs.size() == 1 ) {
    h = hs[0];
    for( auto split_step : h ) { path_cond = path_cond && split_step.cond; }
    // auto pr = std::pair<llvm::BasicBlock*,z3::expr>( preds[0], z3.mk_true() );
    // conds.insert( pr );
    return;
  }
  if( o.print_input > 3 ) for( auto& hp : hs ) cinput::print( std::cerr, hp );
  auto sz = hs[0].size();
  for( auto& old_h: hs ) sz = std::min(sz, old_h.size());
  // unsigned sz = min( h1.size(), h2.size() );
  unsigned i=0;
  for( ; i < sz; i++ ) {
    const split_step& s = hs[0][i];
    bool diff_found = false;
    for( auto& old_h: hs ) {
      if( !( z3::eq( s.cond, old_h[i].cond ) ) ) {
        diff_found = true;
        break;
      }
    }
    if( diff_found ) break;
    h.push_back( s );
  }
  if( i == sz ) {
    bool longer_found = false;
    for( auto& old_h: hs ) if( sz != old_h.size() ) longer_found = true;
    if( longer_found ) {
      cinput_error( "histories can not be subset of each other!!" );
    }
  }else{
    z3::expr c = z3.mk_false();
    unsigned j = 0;
    for( auto& old_h: hs ) {
      unsigned i1 = i;
      z3::expr c1 = z3.mk_true();
      for(;i1 < old_h.size();i1++) {
        c1 = c1 && old_h[i1].cond;
      }
      auto pr = std::pair<llvm::BasicBlock*,z3::expr>( preds[j++], c1 );
      conds.insert( pr );
      c = c || c1;
    }
    c = c.simplify();
    if( z3::eq( c, z3.mk_true() ) )
      return;
    split_step ss( NULL, 0, 0, c);// todo: can store meaningful information
    h.push_back(ss);
  }
  for( auto split_step : h ) { path_cond = path_cond && split_step.cond; }
}

z3::expr build_program::fresh_int() {
  static unsigned count = 0;
  count++;
  std::string loc_name = "i_" + std::to_string(count);
  z3::expr loc_expr = z3.c.int_const(loc_name.c_str());
  return loc_expr;
}

z3::expr build_program::fresh_bool() {
  static unsigned count = 0;
  count++;
  std::string loc_name = "b_" + std::to_string(count);
  z3::expr loc_expr = z3.c.bool_const(loc_name.c_str());
  return loc_expr;
}

hb_enc::depends_set build_program::get_depends( const llvm::Value* op ) {
  if( llvm::isa<llvm::Constant>(op) ) {
    hb_enc::depends_set data_dep;
    return data_dep; }
  else {
    return local_map.at(op); }
}

hb_enc::depends_set build_program::join_depends_set( hb_enc::depends_set& dep0, hb_enc::depends_set dep1 ) {
  hb_enc::depends_set final_set;
  for(std::set<hb_enc::depends>::iterator it0 = dep0.begin(); it0 != dep0.end(); it0++) {
    z3::expr cond = z3.mk_emptyexpr();
    bool flag = false;
    hb_enc::depends element0 = *it0;
    hb_enc::se_ptr val0 = element0.e;
    for(std::set<hb_enc::depends>::iterator it1 = dep1.begin(); it1 != dep1.end(); it1++) {
      hb_enc::depends element1 = *it1;
      hb_enc::se_ptr val1 = element1.e;
      if( val0 == val1 ) {
        cond || ( element0.cond || element1.cond );
        cond.simplify();
        final_set.insert( hb_enc::depends( val0, cond ) );
        dep1.erase(it1);
        flag = true;
      }
     }
      if( !flag ) {
        final_set.insert( hb_enc::depends( val0, element0.cond ) );}
     }
      if( !dep1.empty() ) {
        for(std::set<hb_enc::depends>::iterator it = dep1.begin(); it != dep1.end(); it++) {
          hb_enc::depends element = *it;
          final_set.insert( hb_enc::depends( element.e, element.cond ) );}
	}
  return final_set;
}

z3::expr build_program::getTerm( const llvm::Value* op ,ValueExprMap& m ) {
  if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
    unsigned bw = c->getBitWidth();
    if( bw == 32 ) {
      int i = readInt( c );
      return z3.c.int_val(i);
    }else if(bw == 1) {
      int i = readInt( c );
      assert( i == 0 || i == 1 );
      if( i == 1 ) z3.mk_true(); else z3.mk_false();
    }else
      cinput_error( "unrecognized constant!" );
  }else if( llvm::isa<llvm::ConstantPointerNull>(op) ) {
    cinput_error( "Constant pointer are not implemented!!" );
    // }else if( LLCAST( llvm::ConstantPointerNull, c, op) ) {
    return z3.c.int_val(0);
  }else if( llvm::isa<llvm::Constant>(op) ) {
    cinput_error( "non int constants are not implemented!!" );
    std::cerr << "un recognized constant!";
    //     // int i = readInt(c);
    //     // return eHandler->mkIntVal( i );
  }else if( llvm::isa<llvm::ConstantFP>(op) ) {
    // const llvm::APFloat& n = c->getValueAPF();
    // double v = n.convertToDouble();
    //return z3.c.real_val(v);
    cinput_error( "Floating point constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantExpr>(op) ) {
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantArray>(op) ) {
    // const llvm::ArrayType* n = c->getType();
    // unsigned len = n->getNumElements();
    //return z3.c.arraysort();
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantStruct>(op) ) {
    // const llvm::StructType* n = c->getType();
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantVector>(op) ) {
    // const llvm::VectorType* n = c->getType();
    cinput_error( "vector constant not implemented!!" );
  }else{
    auto it = m.find( op );
    if( it == m.end() ) {
      llvm::Type* ty = op->getType();
      if( auto i_ty = llvm::dyn_cast<llvm::IntegerType>(ty) ) {
        int bw = i_ty->getBitWidth();
        if(bw == 32 || bw == 64 ) {
          z3::expr i =  fresh_int();
          insert_term_map( op, i, m );
          // m.at(op) = i;
          return i;
        }else if(bw == 1) {
          z3::expr bit =  fresh_bool();
          insert_term_map( op, bit, m );
          // m.at(op) = bit;
          return bit;
        }
      }
      cinput_error("unsupported type!!");
    }
    return it->second;
  }
  return z3.mk_true(); // dummy return to avoid warning
}

z3::expr
build_program::
translateBlock( unsigned thr_id,
                const llvm::BasicBlock* b,
                hb_enc::se_set& prev_events,
                z3::expr& path_cond,
                std::map<const llvm::BasicBlock*,z3::expr>& conds ) {
  assert(b);
  z3::expr block_ssa = z3.mk_true();
  // hb_enc::depends_set data_dep_ses;
  // hb_enc::depends_set ctrl_dep_ses;
  // local_data_dependency local_map;
  z3::expr join_conds = z3.mk_true();
  // std::vector<typename EHandler::expr> blockTerms;
  // //iterate over instructions
  for( const llvm::Instruction& Iobj : b->getInstList() ) {
    const llvm::Instruction* I = &(Iobj);
    assert( I );
    bool recognized = false;
    if( auto store = llvm::dyn_cast<llvm::StoreInst>(I) ) {
      llvm::Value* addr = store->getOperand(1);
      std::string loc_name = getInstructionLocationName( I );
      z3::expr val = getTerm( store->getOperand(0), m );
      hb_enc::se_set new_events;
      if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
        cssa::variable gv = p->get_global( (std::string)(g->getName()) );
        auto wr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                             gv, loc_name, hb_enc::event_t::w );
        new_events.insert( wr );
        data_dep_ses.insert( hb_enc::depends( wr, path_cond ) );
	local_map.insert( std::make_pair( I, data_dep_ses ));
	p->data_dependency[wr].insert( data_dep_ses.begin(), data_dep_ses.end() );
        block_ssa = block_ssa && ( wr->v == val );
      }else{
        if( !llvm::isa<llvm::PointerType>( addr->getType() ) )
          cinput_error( "non pointer dereferenced!" );
        // How to deal with null??
        auto& p_set = points_to.at(addr).p_set;
        for( std::pair< z3::expr, cssa::variable > a_pair : p_set ) {
          z3::expr c = a_pair.first;
          cssa::variable gv = a_pair.second;
          z3::expr path_cond_c = path_cond && c;
          auto wr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond_c,
                               gv, loc_name, hb_enc::event_t::w );
          block_ssa = block_ssa && implies( c , ( wr->v == val ) );
          new_events.insert( wr);
        }
        if( points_to.at(addr).has_null() ) {
          z3::expr null_cond = points_to.at(addr).get_null_cond();
          // assert that this can not happen;
          cinput_error( "potentially null access in not supported yet!" );
        }
      }
      p->add_event( thr_id, new_events );
      prev_events = new_events;
      assert( !recognized );recognized = true;
    }

    if( llvm::isa<llvm::UnaryInstruction>(I) ) {
      if( auto load = llvm::dyn_cast<llvm::LoadInst>(I) ) {
        llvm::Value* addr = load->getOperand(0);
        std::string loc_name = getInstructionLocationName( I );
        z3::expr l_v = getTerm( I, m);
        hb_enc::se_set new_events;
        if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
          cssa::variable gv = p->get_global( (std::string)(g->getName()) );
          auto rd = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                               gv, loc_name, hb_enc::event_t::r );
          data_dep_ses.insert( hb_enc::depends( rd, path_cond ) );
          local_map.insert( std::make_pair( I, data_dep_ses ));
          new_events.insert( rd );
          block_ssa = block_ssa && ( rd->v == l_v);
        }else{
          if( !llvm::isa<llvm::PointerType>(addr->getType()) )
            cinput_error( "non pointer dereferenced!" );
          auto& potinted_set = points_to.at(addr).p_set;
          for( std::pair< z3::expr, cssa::variable > a_pair : potinted_set ){
            z3::expr c = a_pair.first;
            cssa::variable gv = a_pair.second;
            z3::expr path_cond_c = path_cond && c;
            auto rd = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond_c,
                                 gv, loc_name, hb_enc::event_t::r );
            block_ssa = block_ssa && implies( c , ( rd->v == l_v ) );
            new_events.insert( rd );
          }
          if( points_to.at(addr).has_null() ) {
            z3::expr null_cond = points_to.at(addr).get_null_cond();
            // assert that this can not happen;
            cinput_error( "potentially null access in not supported yet!" );
          }
        }
        p->add_event( thr_id, new_events );
        prev_events = new_events;
        assert( !recognized );recognized = true;
      }else if( auto alloc = llvm::dyn_cast<llvm::AllocaInst>(I) ) {
        if( alloc->isArrayAllocation() ||
            !alloc->getType()->getElementType()->isIntegerTy() ) {
          cinput_error( "only pointers to intergers is allowed!" );
        }
        std::string alloc_name = "alloc__" + getInstructionLocationName( I );
        z3::sort sort = llvm_to_z3_sort( z3.c,
                                         alloc->getType()->getElementType() );
        p->add_allocated( alloc_name, sort );
        cssa::variable v = p->get_allocated( alloc_name );
        points_set_t p_set = { std::make_pair( z3.mk_true(), v ) };
        pointing pointing_( p_set );
        points_to.insert( std::make_pair(alloc,pointing_) );//todo: remove ugly
        assert( !recognized );recognized = true;
      }else{
        cinput_error("unsupported uniary instruction!!" << "\n");
      }
    }

    if( auto bop = llvm::dyn_cast<llvm::BinaryOperator>(I) ) {
      hb_enc::depends_set dep_ses0;
      hb_enc::depends_set dep_ses1;
      llvm::Value* op0 = bop->getOperand( 0 );
      llvm::Value* op1 = bop->getOperand( 1 );
      z3::expr a = getTerm( op0, m );
      z3::expr b = getTerm( op1, m );
      unsigned op = bop->getOpcode();
      switch( op ) {
      case llvm::Instruction::Add : insert_term_map( I, a+b,     m); break;
      case llvm::Instruction::Sub : insert_term_map( I, a-b,     m); break;
      case llvm::Instruction::Mul : insert_term_map( I, a*b,     m); break;
      case llvm::Instruction::Xor : insert_term_map( I, a xor b, m); break;
      case llvm::Instruction::SDiv: insert_term_map( I, a/b,     m); break;
      default: {
        const char* opName = bop->getOpcodeName();
        cinput_error("unsupported instruction " << opName << " occurred!!");
      }
      }
         dep_ses0 = get_depends( op0 );
         dep_ses1 = get_depends( op1 );
         data_dep_ses = join_depends_set( dep_ses0, dep_ses1 );
         assert( !recognized );recognized = true;
       }

    if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
      hb_enc::depends_set dep_ses0;
      hb_enc::depends_set dep_ses1;
      llvm::Value* lhs = cmp->getOperand( 0 );
      llvm::Value* rhs = cmp->getOperand( 1 );
      z3::expr l = getTerm( lhs, m );
      z3::expr r = getTerm( rhs, m );
      llvm::CmpInst::Predicate pred = cmp->getPredicate();

      switch( pred ) {
      case llvm::CmpInst::ICMP_EQ  : insert_term_map( I, l==r, m ); break;
      case llvm::CmpInst::ICMP_NE  : insert_term_map( I, l!=r, m ); break;
      case llvm::CmpInst::ICMP_UGT : insert_term_map( I, l> r, m ); break;
      case llvm::CmpInst::ICMP_UGE : insert_term_map( I, l>=r, m ); break;
      case llvm::CmpInst::ICMP_ULT : insert_term_map( I, l< r, m ); break;
      case llvm::CmpInst::ICMP_ULE : insert_term_map( I, l<=r, m ); break;
      case llvm::CmpInst::ICMP_SGT : insert_term_map( I, l> r, m ); break;
      case llvm::CmpInst::ICMP_SGE : insert_term_map( I, l>=r, m ); break;
      case llvm::CmpInst::ICMP_SLT : insert_term_map( I, l< r, m ); break;
      case llvm::CmpInst::ICMP_SLE : insert_term_map( I, l<=r, m ); break;
      default: {
        cinput_error( "unsupported predicate in compare " << pred << "!!");
      }
      }
      dep_ses0 = get_depends( lhs );
      dep_ses1 = get_depends( rhs );
      data_dep_ses = join_depends_set( dep_ses0, dep_ses1 );
      assert( !recognized );recognized = true;
    }
    if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
      hb_enc::depends_set temp;
      std::vector<hb_enc::depends_set> dep_ses;
      assert( conds.size() > 1 ); //todo:if not,review initialization of conds
      unsigned num = phi->getNumIncomingValues();
      std::map<const llvm::Value*,bool> cond;
      if( phi->getType()->isIntegerTy() ) {
        z3::expr phi_cons = z3.mk_false();
        z3::expr ov = getTerm(I,m);

        for ( unsigned i = 0 ; i < num ; i++ ) {
          const llvm::BasicBlock* b = phi->getIncomingBlock(i);
          const llvm::Value* v_ = phi->getIncomingValue(i);
          z3::expr v = getTerm (v_, m );
          temp = get_depends( v_ );
	  for(std::set<hb_enc::depends>::iterator it = temp.begin(); it != temp.end(); it++) {
	    hb_enc::depends element = *it;
	    element.cond = element.cond && conds.at(b);
	  }
	  dep_ses.push_back( temp );
	  z3::expr phi_cons = phi_cons || (conds.at(b) && ov == v);
        }
        if ( num == 1 )
          data_dep_ses = dep_ses.at(0);
        else if ( num == 2 )
          data_dep_ses = join_depends_set( dep_ses.at(0), dep_ses.at( 1 ) );
        else if ( num > 2 ) {
          data_dep_ses = join_depends_set( dep_ses.at(0), dep_ses.at( 1 ) );
          for ( unsigned i = 2 ; i < num ; i++ ) {
	    data_dep_ses = join_depends_set( data_dep_ses, dep_ses.at( i ) );
          }
        }
        block_ssa = block_ssa && phi_cons;
        assert( !recognized );recognized = true;
      }else{
        cinput_error("phi nodes with non integers not supported !!");
      }
     }

    if( auto ret = llvm::dyn_cast<llvm::ReturnInst>(I) ) {
      llvm::Value* v = ret->getReturnValue();
      if( v ) {
  //       typename EHandler::expr retTerm = getTerm( v, m );
  //       //todo: use retTerm somewhere
      }
      if( o.print_input > 0 ) cinput_warning( "return value ignored!!" );
      assert( !recognized );recognized = true;
    }
    if( llvm::isa<llvm::UnreachableInst>(I) ) {
      cinput_error( "unreachable instruction reached!");
      // assert( !recognized );recognized = true;
    }

    UNSUPPORTED_INSTRUCTIONS( InvokeInst,      I );
    UNSUPPORTED_INSTRUCTIONS( IndirectBrInst,  I );
    // UNSUPPORTED_INSTRUCTIONS( UnreachableInst, I );

    if( auto br = llvm::dyn_cast<llvm::BranchInst>(I) ) {
      z3::expr bit = getTerm( br->getCondition(), m);
      block_to_exit_bit.insert( std::make_pair(b,bit) );
      assert( !recognized );recognized = true;
      if(br->isConditional()) {
	std::map<const llvm::Value*,bool> cond;
        for( const llvm::Instruction& Iobj : b->getInstList() ) {
          const llvm::Instruction* I = &(Iobj);
          for( unsigned i = 0; i < I->getNumOperands(); i++){
            const llvm::Value* op = I->getOperand(i);
	    if( llvm::isa<llvm::Constant>(op) ){ continue; }
	    else {
	      auto pr = std::pair<const llvm::Value*, bool>( op, br->getCondition());
	      cond.insert(pr);
	    }
	  }
	}
      }
    }
    UNSUPPORTED_INSTRUCTIONS( SwitchInst,  I );
    // if( llvm::isa<llvm::SwitchInst>(I) ) {
    //   cinput_error( "switch statement not supported yet!!");
    //   assert( !recognized );recognized = true;
    // }
    if( auto call = llvm::dyn_cast<llvm::CallInst>(I) ) {
      if( llvm::isa<llvm::DbgValueInst>(I) ||
          llvm::isa<llvm::DbgDeclareInst>(I) ) {
        // Ignore debug instructions
      }else{
        llvm::Function* fp = call->getCalledFunction();
        unsigned argnum = call->getNumArgOperands();
        if( fp != NULL && ( fp->getName() == "_Z5fencev" ) ) {
          std::string loc_name = "fence__" + getInstructionLocationName( I );
          auto barr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                                 loc_name, hb_enc::event_t::barr );
          p->add_event( thr_id, barr );
          prev_events.clear(); prev_events.insert( barr );
        }else if( fp != NULL && ( fp->getName() == "pthread_create" ) &&
                  argnum == 4 ) {
          llvm::Value* thr_ptr = call->getArgOperand(0);
          llvm::Value* v = call->getArgOperand(2);
          if( !v->hasName() ) {
            v->getType()->print( llvm::outs() ); llvm::outs() << "\n";
            cinput_error( "unnamed call to function pointers is not supported!!");
          }
          std::string loc_name = "create__" + getInstructionLocationName( I );
          auto barr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                                 loc_name, hb_enc::event_t::barr_b );
          std::string fname = (std::string)v->getName();
          ptr_to_create[thr_ptr] = fname;
          p->add_create( thr_id, barr, fname );
          prev_events.clear(); prev_events.insert( barr );
          // create
        }else if( fp != NULL && ( fp->getName() == "pthread_join" ) ) {
          // join
          llvm::Value* val = call->getArgOperand(0);
          auto thr_ptr = llvm::cast<llvm::LoadInst>(val)->getOperand(0);
          auto fname = ptr_to_create.at(thr_ptr);
          ptr_to_create.erase( thr_ptr );
          z3::expr join_cond = fresh_bool();
          path_cond = path_cond && join_cond;
          std::string loc_name = "join__" + getInstructionLocationName( I );
          auto barr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                                 loc_name, hb_enc::event_t::barr_a );
          p->add_join( thr_id, barr, join_cond, fname);
          join_conds = join_conds && join_cond;
          prev_events.clear(); prev_events.insert( barr );
        }else{
          if( o.print_input > 0 )
            llvm::outs() << "Unknown function" << fp->getName() << "\n";
          cinput_error( "Unknown function called");
        }
      }
      assert( !recognized );recognized = true;
    }
    if( !recognized ) cinput_error( "----- failed to recognize!!");
  }
  split_step ss( NULL, 0, 0, join_conds);
  block_to_split_stack.at(b).push_back( ss );
  return block_ssa;
}

bool build_program::runOnFunction( llvm::Function &f ) {
  std::string name = (std::string)f.getName();
  unsigned thread_id = p->add_thread( name );
  hb_enc::se_set prev_events, final_prev_events;
  if( o.print_input > 0 ) std::cout << "Processing Function: " << name << "\n";

  initBlockCount( f, block_to_id );


  z3::expr start_bit = fresh_bool();
  auto start = mk_se_ptr( hb_encoding, thread_id, prev_events, start_bit,
                          name, hb_enc::event_t::barr );
  p->set_start_event( thread_id, start, start_bit );
  if( name == "main" ) {
    p->create_map[ "main" ] = *p->init_loc.begin();
    hb_enc::se_ptr post_loc = *p->post_loc.begin();
    auto pr = std::make_pair( post_loc, z3.mk_true() );
    p->join_map.insert( std::make_pair( "main" , pr) );
  }

  prev_events.insert( start );

  std::vector< split_history     > final_histories;
  std::vector< llvm::BasicBlock* > final_preds;

  for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* src = &(*it);
    if( o.print_input > 0 ) src->print( llvm::outs() );
    if( src->hasName() && (src->getName() == "dummy")) continue;

    std::vector< split_history > histories; // needs to be ref
    std::vector<llvm::BasicBlock*> preds;
    for(auto PI = llvm::pred_begin(src),E = llvm::pred_end(src);PI != E;++PI){
      llvm::BasicBlock *prev = *PI;
      split_history h = block_to_split_stack[prev];
      if( llvm::isa<llvm::BranchInst>( prev->getTerminator() ) ) {
        z3::expr b = block_to_exit_bit.at(prev);
        // llvm::TerminatorInst *term= prev->getTerminator();
        unsigned succ_num = PI.getOperandNo();
        assert( succ_num );
        if( succ_num == 2 ) b = !b;
        split_step ss(prev, block_to_id[prev], succ_num, b);
        h.push_back(ss);
      }
      histories.push_back(h);
      hb_enc::se_set& prev_trail = block_to_trailing_events.at(prev);
      prev_events.insert( prev_trail.begin(), prev_trail.end() );
      preds.push_back( prev );
    }
    if( src == &(f.getEntryBlock()) ) {
      split_history h;
      split_step ss( NULL, 0, 0, start_bit ); // todo : second param??
      h.push_back( ss );
      histories.push_back( h );
    }
    split_history h;
    std::map<const llvm::BasicBlock*, z3::expr> conds;
    z3::expr path_cond = z3.mk_true();
    join_histories( preds, histories, h, path_cond, conds);
    block_to_split_stack[src] = h;

    if( src->hasName() && (src->getName() == "err") ) {
      p->append_property( thread_id, !path_cond );
      // for( auto pair_ : conds ) p->append_property( thread_id, pair_.second );
      continue;
    }
    z3::expr ssa = translateBlock(thread_id, src, prev_events,path_cond,conds);
    block_to_trailing_events[src] = prev_events;
    if( llvm::isa<llvm::ReturnInst>( src->getTerminator() ) ) {
      final_prev_events.insert( prev_events.begin(), prev_events.end() );
      final_histories.push_back( h );
      final_preds.push_back( src );
    }
    prev_events.clear();
    p->append_ssa( thread_id, ssa );
    if( o.print_input > 0 ) helpers::debug_print(ssa );
  }

  split_history final_h;
  std::map<const llvm::BasicBlock*, z3::expr> conds;
  z3::expr exit_cond = z3.mk_true();
  join_histories( final_preds, final_histories, final_h, exit_cond, conds);

  auto final = mk_se_ptr( hb_encoding, thread_id, final_prev_events, exit_cond,
                          name+"_final", hb_enc::event_t::barr );
  p->set_final_event( thread_id, final, exit_cond );

  return false;
}
