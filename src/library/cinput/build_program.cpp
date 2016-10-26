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
    if( preds.size() == 1 ) {
      auto pr = std::pair<llvm::BasicBlock*,z3::expr>( preds[0], z3.mk_true() );
      conds.insert( pr );
    }
    return;
  }
  if( o.print_input > 3 ) for( auto& hp : hs ) cinput::print( std::cerr, hp );
  auto sz = hs[0].size();
  for( auto& old_h: hs ) sz = std::min(sz, old_h.size());
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

// function that takes a history as input and returns vector of z3::expr
void split_history_to_exprs( const split_history& h,
                             std::vector<z3::expr> history_exprs ) {
  history_exprs.clear();
  for( auto& step : h ) history_exprs.push_back( step.cond );
}


// z3::expr build_program::fresh_int( helpers::z3interf& z3_ ) {
//   return z3_.get_fresh_int();
//   // static unsigned count = 0;
//   // count++;
//   // std::string loc_name = "i_" + std::to_string(count);
//   // z3::expr loc_expr = z3_.c.int_const(loc_name.c_str());
//   // return loc_expr;
// }

// z3::expr build_program::fresh_bool( helpers::z3interf& z3_ ) {
//   return z3_.get_fresh_bool();
//   // static unsigned count = 0;
//   // count++;
//   // std::string loc_name = "b_" + std::to_string(count);
//   // z3::expr loc_expr = z3_.c.bool_const(loc_name.c_str());
//   // return loc_expr;
// }

hb_enc::depends_set build_program::get_depends( const llvm::Value* op ) {
  if( llvm::isa<llvm::Constant>(op) ) {
    hb_enc::depends_set data_dep;
    return data_dep; }
  else {
    return local_map.at(op); }
}

hb_enc::depends_set build_program::get_ctrl( const llvm::BasicBlock* b ) {
  if( b->empty()) {
    hb_enc::depends_set ctrl_dep;
    return ctrl_dep; }
  else {
    return local_ctrl.at(b); }
}



z3::expr build_program::get_term( helpers::z3interf& z3_,
                                  const llvm::Value* op, ValueExprMap& vmap ) {
  if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
    unsigned bw = c->getBitWidth();
    if( bw == 32 ) {
      int i = readInt( c );
      return z3_.c.int_val(i);
    }else if(bw == 1) {
      int i = readInt( c );
      assert( i == 0 || i == 1 );
      if( i == 1 ) z3_.mk_true(); else z3_.mk_false();
    }else
      cinput_error( "unrecognized constant!" );

  }else if( llvm::isa<llvm::ConstantPointerNull>(op) ) {
    cinput_error( "Constant pointer are not implemented!!" );
    // }else if( LLCAST( llvm::ConstantPointerNull, c, op) ) {
    return z3_.c.int_val(0);
  }else if( llvm::isa<llvm::UndefValue>(op) ) {
    llvm::Type* ty = op->getType();
    if( auto i_ty = llvm::dyn_cast<llvm::IntegerType>(ty) ) {
      int bw = i_ty->getBitWidth();
      if(bw == 32 || bw == 64 ) { return z3_.get_fresh_int();
      }else if(      bw == 1  ) { return z3_.get_fresh_bool();
      }
    }
    cinput_error("unsupported type!!");
  }else if( llvm::isa<llvm::Constant>(op) ) {
    cinput_error( "non int constants are not implemented!!" );
    std::cerr << "un recognized constant!";
    //     // int i = readInt(c);
    //     // return eHandler->mkIntVal( i );
  }else if( llvm::isa<llvm::ConstantFP>(op) ) {
    // const llvm::APFloat& n = c->getValueAPF();
    // double v = n.convertToDouble();
    //return z3_.c.real_val(v);
    cinput_error( "Floating point constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantExpr>(op) ) {
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantArray>(op) ) {
    // const llvm::ArrayType* n = c->getType();
    // unsigned len = n->getNumElements();
    //return z3_.c.arraysort();
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantStruct>(op) ) {
    // const llvm::StructType* n = c->getType();
    cinput_error( "case for constant not implemented!!" );
  }else if( llvm::isa<llvm::ConstantVector>(op) ) {
    // const llvm::VectorType* n = c->getType();
    cinput_error( "vector constant not implemented!!" );
  }else{
    auto it = vmap.find( op );
    if( it == vmap.end() ) {
      llvm::Type* ty = op->getType();
      if( auto i_ty = llvm::dyn_cast<llvm::IntegerType>(ty) ) {
        int bw = i_ty->getBitWidth();
        if(bw == 32 || bw == 64 ) {
          z3::expr i =  z3_.get_fresh_int();
          insert_term_map( op, i, vmap );
          // m.at(op) = i;
          return i;
        }else if(bw == 1) {
          z3::expr bit =  z3_.get_fresh_bool();
          insert_term_map( op, bit, vmap );
          // m.at(op) = bit;
          return bit;
        }
      }
      cinput_error("unsupported type!!");
    }
    return it->second;
  }
  return z3_.mk_true(); // dummy return to avoid warning
}


hb_enc::o_tag_t
translate_ordering_tags( llvm::AtomicOrdering ord ) {
  switch( ord ) {
  case llvm::AtomicOrdering::NotAtomic: return hb_enc::o_tag_t::na; break;
  case llvm::AtomicOrdering::Unordered: return hb_enc::o_tag_t::uo; break;
  case llvm::AtomicOrdering::Monotonic: return hb_enc::o_tag_t::mon; break;
  case llvm::AtomicOrdering::Acquire: return hb_enc::o_tag_t::acq; break;
  case llvm::AtomicOrdering::Release: return hb_enc::o_tag_t::rls; break;
  case llvm::AtomicOrdering::AcquireRelease: return hb_enc::o_tag_t::acqrls; break;
  case llvm::AtomicOrdering::SequentiallyConsistent: return hb_enc::o_tag_t::sc; break;
  default:
    cinput_error( "unrecognized C++ ordering returned!!" );
  }
  return hb_enc::o_tag_t::na; // dummy return;
}

z3::expr
build_program::
translateBlock( unsigned thr_id,
                const llvm::BasicBlock* b,
                hb_enc::se_set& prev_events,
                z3::expr& path_cond,
                std::vector< z3::expr >& history,
                std::map<const llvm::BasicBlock*,z3::expr>& conds,
                std::map<const hb_enc::se_ptr, z3::expr>& branch_conds ) {

  std::string loc_name = "block__" + std::to_string(thr_id)
                             + "__"+ std::to_string(block_to_id[b]);
  auto block = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                          history, loc_name, hb_enc::event_t::block,
                          branch_conds );
  p->add_event( thr_id, block );
  prev_events.clear(); prev_events.insert( block );

  assert(b);
  z3::expr block_ssa = z3.mk_true();
  z3::expr join_conds = z3.mk_true();
  // hb_enc::depends_set ctrl;
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
                             history, gv, loc_name, hb_enc::event_t::w );
        wr->o_tag = translate_ordering_tags( store->getOrdering());
        new_events.insert( wr );
        const auto& data_dep_set = get_depends( store->getOperand(0) );
        local_map.insert( std::make_pair( I, data_dep_set ));
	wr->set_data_dependency( data_dep_set );
	wr->set_ctrl_dependency( get_ctrl(b) );
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
          auto wr = mk_se_ptr( hb_encoding, thr_id, prev_events,path_cond_c,
                               history, gv, loc_name, hb_enc::event_t::w );
          wr->o_tag = translate_ordering_tags( store->getOrdering());
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
                               history, gv, loc_name, hb_enc::event_t::r );
          rd->o_tag = translate_ordering_tags( load->getOrdering());
          // todo: << replace path cond ~> true??
          // local_map[I].insert( hb_enc::depends( rd, path_cond ) );
          local_map[I].insert( hb_enc::depends( rd, z3.mk_true() ) );
          rd->set_ctrl_dependency( get_ctrl(b) );
          new_events.insert( rd );
          block_ssa = block_ssa && ( rd->v == l_v);
        }else{
          if( !llvm::isa<llvm::PointerType>(addr->getType()) )
            cinput_error( "non pointer dereferenced!" );
          auto& potinted_set = points_to.at(addr).p_set;
          for( std::pair< z3::expr, cssa::variable > a_pair : potinted_set ) {
            z3::expr c = a_pair.first;
            cssa::variable gv = a_pair.second;
            z3::expr path_cond_c = path_cond && c;
            auto rd = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond_c,
                                 history, gv, loc_name, hb_enc::event_t::r );
            rd->o_tag = translate_ordering_tags( load->getOrdering());
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
      //  dep_ses0;
      // hb_enc::depends_set dep_ses1;
      llvm::Value* op0 = bop->getOperand( 0 );
      llvm::Value* op1 = bop->getOperand( 1 );
      z3::expr a = getTerm( op0, m );
      z3::expr b = getTerm( op1, m );
      unsigned op = bop->getOpcode();
      switch( op ) {
      case llvm::Instruction::Add : insert_term_map( I, a+b,     m); break;
      case llvm::Instruction::Sub : insert_term_map( I, a-b,     m); break;
      case llvm::Instruction::Mul : insert_term_map( I, a*b,     m); break;
      case llvm::Instruction::Xor :
        insert_term_map( I, helpers::_xor(a,b), m); break;
      case llvm::Instruction::SDiv: insert_term_map( I, a/b,     m); break;
      default: {
        const char* opName = bop->getOpcodeName();
        cinput_error("unsupported instruction " << opName << " occurred!!");
      }
      }
      hb_enc::depends_set dep_ses0 = get_depends( op0 );
      hb_enc::depends_set dep_ses1 = get_depends( op1 );
      hb_enc::join_depends_set( dep_ses0, dep_ses1, local_map[I] );
      assert( !recognized );recognized = true;
    }

    if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
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
      hb_enc::depends_set dep_ses0 = get_depends( lhs );
      hb_enc::depends_set dep_ses1 = get_depends( rhs );
      hb_enc::join_depends_set( dep_ses0, dep_ses1, local_map[I] );
      assert( !recognized );recognized = true;
    }
    if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
      std::vector<hb_enc::depends_set> dep_ses;
      assert( conds.size() > 1 ); //todo:if not,review initialization of conds
      unsigned num = phi->getNumIncomingValues();
      if( phi->getType()->isIntegerTy() ) {
        z3::expr phi_cons = z3.mk_false();
        z3::expr ov = getTerm(I,m);

        for ( unsigned i = 0 ; i < num ; i++ ) {
          const llvm::BasicBlock* b = phi->getIncomingBlock(i);
          const llvm::Value* v_ = phi->getIncomingValue(i);
          z3::expr v = getTerm (v_, m );
          phi_cons = phi_cons || (conds.at(b) && ov == v);
          hb_enc::depends_set temp;
          hb_enc::pointwise_and( get_depends( v_ ), conds.at(b), temp );
	  dep_ses.push_back( temp );
        }
        hb_enc::join_depends_set( dep_ses, local_map[I] );
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
      if( br->isUnconditional() ) {
        z3::expr bit = z3.get_fresh_bool();
        block_to_exit_bit.insert( std::make_pair(b,bit) );
      }else{
        z3::expr bit = getTerm( br->getCondition(), m);
        block_to_exit_bit.insert( std::make_pair(b,bit) );
      }
      assert( !recognized );recognized = true;
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
                                 history, loc_name, hb_enc::event_t::barr );
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
                                 history, loc_name, hb_enc::event_t::barr_b );
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
          z3::expr join_cond = z3.get_fresh_bool();
          path_cond = path_cond && join_cond;
          std::string loc_name = "join__" + getInstructionLocationName( I );
          auto barr = mk_se_ptr( hb_encoding, thr_id, prev_events, path_cond,
                                 history, loc_name, hb_enc::event_t::barr_a );
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

void
join_depends_set( const std::map<const llvm::BasicBlock*, hb_enc::depends_set>& deps,
                  const std::map<const llvm::BasicBlock*,z3::expr>& conds,
                  hb_enc::depends_set& result ) {
  result.clear();
  for( auto& pr : deps ) {
    const llvm::BasicBlock* b = pr.first;
    const hb_enc::depends_set& dep = pr.second;
    // tara::debug_print( std::cout, dep );
    z3::expr cond = conds.at(b);
    hb_enc::depends_set tmp;
    hb_enc::pointwise_and( dep, cond, tmp );
    hb_enc::join_depends_set( tmp, result );
  }
  // std::cout << "--- >> ";
  // tara::debug_print( std::cout, result );
}

bool build_program::runOnFunction( llvm::Function &f ) {
  std::string name = (std::string)f.getName();
  unsigned thread_id = p->add_thread( name );
  hb_enc::se_set prev_events, final_prev_events;
  if( o.print_input > 0 ) std::cout << "Processing Function: " << name << "\n";

  initBlockCount( f, block_to_id );


  z3::expr start_bit = z3.get_fresh_bool();
  std::vector< z3::expr > history = { start_bit };
  auto start = mk_se_ptr( hb_encoding, thread_id, prev_events, start_bit,
                          history, name, hb_enc::event_t::barr );
  p->set_start_event( thread_id, start, start_bit );
  if( name == "main" ) {
    p->create_map[ "main" ] = p->init_loc;
    hb_enc::se_ptr post_loc = p->post_loc;
    auto pr = std::make_pair( post_loc, z3.mk_true() );
    p->join_map.insert( std::make_pair( "main" , pr) );
  }

  prev_events.insert( start );

  std::vector< split_history     > final_histories;
  std::vector< llvm::BasicBlock* > final_preds;

  for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* src = &(*it);
    std::map<const llvm::BasicBlock*,hb_enc::depends_set> ctrl_ses;
    if( o.print_input > 0 ) {
      std::cout << "=====================================================\n";
      std::cout << "Processing block" << " block__" << thread_id << "__"
                << block_to_id[src] << "\n";
      src->print( llvm::outs() );
    }
    if( src->hasName() && (src->getName() == "dummy")) continue;

    std::vector< split_history > histories; // needs to be ref
    std::vector<llvm::BasicBlock*> preds;
    std::map<const hb_enc::se_ptr, z3::expr > branch_conds;
    // z3::expr branch_cond = z3.mk_true();
    for(auto PI = llvm::pred_begin(src),E = llvm::pred_end(src);PI != E;++PI) {
      llvm::BasicBlock *prev = *PI;
      split_history h = block_to_split_stack[prev];
      z3::expr prev_cond = z3.mk_true();
      if( llvm::isa<llvm::BranchInst>( prev->getTerminator() ) ) {
        z3::expr b = block_to_exit_bit.at(prev);
        // llvm::TerminatorInst *term= prev->getTerminator();
        unsigned succ_num = PI.getOperandNo();
        // assert( succ_num );
        llvm::BranchInst* br  = (llvm::BranchInst*)(prev->getTerminator());
        hb_enc::depends_set ctrl_temp;
        if( !br->isUnconditional() ) {
          llvm::Value* op = prev->getTerminator()->getOperand(0);
          const auto& ctrl_temp0 = get_depends( op );
          const auto& ctrl_temp1 = get_ctrl(prev);
          hb_enc::join_depends_set( ctrl_temp1 , ctrl_temp0, ctrl_temp );
        }else if( br->isUnconditional() ) {
	  ctrl_temp = get_ctrl(prev);
        }
        assert( br->getOperand(succ_num) == src );
        // std::cout << br->getNumOperands() << "\n";
        if( br->getNumOperands() >= 2 ) {
          if( succ_num == 1 ) b = !b;
          split_step ss(prev, block_to_id[prev], succ_num, b);
          h.push_back(ss);
          prev_cond = b; //todo: hack needs streamlining
        }
        ctrl_ses[prev] = ctrl_temp;
      }
      histories.push_back(h);
      hb_enc::se_set& prev_trail = block_to_trailing_events.at(prev);
      prev_events.insert( prev_trail.begin(), prev_trail.end() );
      for( auto& prev_e : prev_trail ) {
        if( branch_conds.find( prev_e ) != branch_conds.end() ) {
          prev_cond = prev_cond || branch_conds.at(prev_e);
        }
        branch_conds.insert( std::make_pair( prev_e, prev_cond) );
      }
      preds.push_back( prev );
    }
    if( src == &(f.getEntryBlock()) ) {
      split_history h;
      split_step ss( NULL, 0, 0, start_bit ); // todo : second param??
      h.push_back( ss );
      histories.push_back( h );
      branch_conds.insert( std::make_pair( start, z3.mk_true()) );
    }
    split_history h;
    std::map<const llvm::BasicBlock*, z3::expr> conds;
    z3::expr path_cond = z3.mk_true();
    join_histories( preds, histories, h, path_cond, conds);
    block_to_split_stack[src] = h;

    local_ctrl[src].clear();
    if( o.mm == mm_t::rmo )
      join_depends_set( ctrl_ses, conds, local_ctrl[src] );

    if( src->hasName() && (src->getName() == "err") ) {
      p->append_property( thread_id, !path_cond );
      // for( auto pair_ : conds ) p->append_property( thread_id, pair_.second );
      continue;
    }
    std::vector<z3::expr> history_exprs;
    split_history_to_exprs( h, history_exprs );
    z3::expr ssa = translateBlock( thread_id, src, prev_events,
                                   path_cond, history_exprs, conds,
                                   branch_conds);
    block_to_trailing_events[src] = prev_events;
    if( llvm::isa<llvm::ReturnInst>( src->getTerminator() ) ) {
      final_prev_events.insert( prev_events.begin(), prev_events.end() );
      final_histories.push_back( h );
      final_preds.push_back( src );
    }
    prev_events.clear();
    p->append_ssa( thread_id, ssa );
    if( o.print_input > 0 ) {
      std::cout << "path cond : "<< path_cond << "\n";
      std::cout << "path ssa  : \n";
      tara::debug_print(ssa );
      std::cout << "\n";
    }
  }

  split_history final_h;
  std::map<const llvm::BasicBlock*, z3::expr> conds;
  z3::expr exit_cond = z3.mk_true();
  join_histories( final_preds, final_histories, final_h, exit_cond, conds);

  std::vector<z3::expr> history_exprs;
  split_history_to_exprs( final_h, history_exprs );

  auto final = mk_se_ptr( hb_encoding, thread_id, final_prev_events, exit_cond,
                          history_exprs, name+"_final", hb_enc::event_t::barr );

  p->set_final_event( thread_id, final, exit_cond );

  if( o.print_input > 1 ) p->print_dependency( o.out());

  return false;
}
