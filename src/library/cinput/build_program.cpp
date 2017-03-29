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
#include <stdexcept>
#include <z3.h>
#include "helpers/z3interf.h"

// #pragma GCC optimize ("no-rtti")
#include "build_program.h"
using namespace tara;
using namespace tara::cinput;
using namespace tara::helpers;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// pragam'ed to aviod warnings due to llvm included files

#include "llvm/IRReader/IRReader.h"

#include "llvm/IR/InstIterator.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/Pass.h"
// #include "llvm/PassManager.h"
// #include "llvm/IR/PassManager.h"
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
build_program::join_histories(const std::vector<const bb*>& preds,
                              const std::vector< split_history >& hs,
                              split_history& h,
                              z3::expr& path_cond,
                              std::map< const bb*, z3::expr >& conds) {
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
      auto pr = std::pair<const bb*,z3::expr>( preds[0], z3.mk_true() );
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
    std::vector<z3::expr> or_vec;// = z3.mk_false();
    unsigned j = 0;
    for( auto& old_h: hs ) {
      unsigned i1 = i;
      z3::expr c1 = z3.mk_true();
      for(;i1 < old_h.size();i1++) {
        c1 = c1 && old_h[i1].cond;
      }
      auto pr = std::pair<const bb*,z3::expr>( preds[j++], c1 );
      conds.insert( pr );
      or_vec.push_back( c1 );
      // c = c || c1;
    }
    z3::expr c = z3.simplify_or_vector( or_vec );
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


//todo: this function returns object not reference too bad programming
//
hb_enc::depends_set build_program::get_depends( const llvm::Value* op ) {
  if( llvm::isa<llvm::Constant>(op) ) {
    hb_enc::depends_set data_dep;
    return data_dep; }
  else {
    return local_map.at(op); }
}

void build_program::join_depends( const llvm::Value* op1,const llvm::Value* op2,
                                  hb_enc::depends_set& result ) {
  hb_enc::depends_set dep_ses0 = get_depends( op1 );
  hb_enc::depends_set dep_ses1 = get_depends( op2 );
  hb_enc::join_depends_set( dep_ses0, dep_ses1, result );
}

hb_enc::depends_set build_program::get_ctrl( const bb* b ) {
  if( b->empty() ) {
    hb_enc::depends_set ctrl_dep;
    return ctrl_dep;
  }else{
    return local_ctrl.at(b);
  }
}



z3::expr build_program::get_term( helpers::z3interf& z3_,
                                  const llvm::Value* op, ValueExprMap& vmap ) {
  if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
    unsigned bw = c->getBitWidth();
    if( bw == 32 ) {
      int i = readInt( c );
      return z3_.c.int_val(i);
    }else if(bw == 1 || bw == 8) { // << ----
      int i = readInt( c );
      assert( i == 0 || i == 1 );
      if( i == 1 ) return z3_.mk_true(); else return z3_.mk_false();
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
    cinput_error("unsupported type: "<< ty << "!!");
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
        }else if(bw == 1 || bw == 8) {
          z3::expr bit =  z3_.get_fresh_bool();
          insert_term_map( op, bit, vmap );
          // m.at(op) = bit;
          return bit;
        }
      }
      ty->print( llvm::errs() ); llvm::errs() << "\n";
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
                const bb* b,
                hb_enc::se_set& prev_events,
                z3::expr& path_cond,
                std::vector< z3::expr >& history,
                std::map<const bb*,z3::expr>& conds,
                std::map<const hb_enc::se_ptr, z3::expr>& branch_conds ) {

  // std::string loc_name = "block__" + std::to_string(thr_id)
  //                            + "__"+ std::to_string(block_to_id[b]);
  // auto block = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond, history,
  //                         loc_name, hb_enc::event_t::block, branch_conds );
  hb_enc::source_loc loc_name = getBlockLocation(b);
  auto block = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond, history,
                          loc_name, hb_enc::event_t::block, branch_conds );
  p->add_event( thr_id, block );
  p->set_c11_rs_heads( block, local_release_heads[b] );

  prev_events.clear(); prev_events.insert( block );

  assert(b);
  z3::expr block_ssa = z3.mk_true();
  z3::expr join_conds = z3.mk_true();

  // collect loop back edges that should be ignored
  bb_set_t ignore_edges;
  if( helpers::exists( loop_ignore_edges, b ) )
    ignore_edges = loop_ignore_edges.at(b);

  for( const llvm::Instruction& Iobj : b->getInstList() ) {
    const llvm::Instruction* I = &(Iobj);
    assert( I );
    bool recognized = false;
    if( auto store = llvm::dyn_cast<llvm::StoreInst>(I) ) {
      llvm::Value* addr = store->getOperand(1);
      auto loc = getInstructionLocation( I );
      z3::expr val = getTerm( store->getOperand(0), m );
      hb_enc::se_set new_events;
      if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
        auto gv = p->get_global( (std::string)(g->getName()) );
        auto wr = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                             history, gv, loc, hb_enc::event_t::w,
                             translate_ordering_tags( store->getOrdering()) );
        new_events.insert( wr );
        const auto& data_dep_set = get_depends( store->getOperand(0) );
        // local_map.insert( std::make_pair( I, data_dep_set ));//todo: why need this ~~~ should be deleted
	wr->set_data_dependency( data_dep_set );
	wr->set_ctrl_dependency( get_ctrl(b) );
	block_ssa = block_ssa && ( wr->wr_v() == val );
      }else{
        if( !llvm::isa<llvm::PointerType>( addr->getType() ) )
          cinput_error( "non pointer dereferenced!" );
        // How to deal with null??
        auto& p_set = points_to.at(addr).p_set;
        for( std::pair< z3::expr, tara::variable > a_pair : p_set ) {
          z3::expr c = a_pair.first;
          auto gv = a_pair.second;
          z3::expr path_cond_c = path_cond && c;
          auto wr = mk_se_ptr( hb_enc, thr_id, prev_events,path_cond_c,
                               history, gv, loc, hb_enc::event_t::w,
                               translate_ordering_tags( store->getOrdering()));
          block_ssa = block_ssa && implies( c , ( wr->wr_v() == val ) );
          new_events.insert( wr);
        }
        if( points_to.at(addr).has_null() ) {
          z3::expr null_cond = points_to.at(addr).get_null_cond();
          // assert that this can not happen;
          cinput_error( "potentially null access in not supported yet!" );
        }
      }
      p->add_event( thr_id, new_events );
      p->set_c11_rs_heads( new_events, local_release_heads[b] );
      prev_events = new_events;
      assert( !recognized );recognized = true;
    }

    if( llvm::isa<llvm::UnaryInstruction>(I) ) {
      if( auto cast = llvm::dyn_cast<llvm::CastInst>(I) ) {
        if( auto trunc = llvm::dyn_cast<llvm::TruncInst>(cast) ) {
          auto v = trunc->getOperand(0);
          if( v->getType()->isIntegerTy(8) && I->getType()->isIntegerTy(1) ) {
            insert_term_map( I, getTerm( v, m), m );
            local_map[I] = get_depends(v);
            assert( !recognized );recognized = true;
          }
        }else if( auto zext = llvm::dyn_cast<llvm::ZExtInst>(I) ) {
          auto v = zext->getOperand(0);
          if( v->getType()->isIntegerTy(1) &&
              (I->getType()->isIntegerTy(8) ||I->getType()->isIntegerTy(32)) ) {
            insert_term_map( I, getTerm( v, m), m );
            local_map[I] = get_depends(v);
            assert( !recognized );recognized = true;
          }
        }
        if( !recognized ) cinput_error( "unspported cast instruction!" );
      }else if( auto load = llvm::dyn_cast<llvm::LoadInst>(I) ) {
        llvm::Value* addr = load->getOperand(0);
        auto loc = getInstructionLocation( I );
        z3::expr l_v = getTerm( I, m);
        hb_enc::se_set new_events;
        if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
          auto gv = p->get_global( (std::string)(g->getName()) );
          auto rd = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                               history, gv, loc, hb_enc::event_t::r,
                               translate_ordering_tags( load->getOrdering()) );
          local_map[I].insert( hb_enc::depends( rd, z3.mk_true() ) );
          rd->set_ctrl_dependency( get_ctrl(b) );
          new_events.insert( rd );
          block_ssa = block_ssa && ( rd->rd_v() == l_v);
        }else{
          if( !llvm::isa<llvm::PointerType>(addr->getType()) )
            cinput_error( "non pointer dereferenced!" );
          auto& potinted_set = points_to.at(addr).p_set;
          for( std::pair< z3::expr, tara::variable > a_pair : potinted_set ) {
            z3::expr c = a_pair.first;
            auto gv = a_pair.second;
            z3::expr path_cond_c = path_cond && c;
            auto rd = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond_c,
                                 history, gv, loc, hb_enc::event_t::r,
                                 translate_ordering_tags( load->getOrdering()));
            block_ssa = block_ssa && implies( c , ( rd->rd_v() == l_v ) );
            new_events.insert( rd );
          }
          if( points_to.at(addr).has_null() ) {
            z3::expr null_cond = points_to.at(addr).get_null_cond();
            // assert that this can not happen;
            cinput_error( "potentially null access in not supported yet!" );
          }
        }
        p->add_event( thr_id, new_events);
        p->set_c11_rs_heads( new_events, local_release_heads[b] );
        prev_events = new_events;
        assert( !recognized );recognized = true;
      }else if( auto alloc = llvm::dyn_cast<llvm::AllocaInst>(I) ) {
        if( alloc->isArrayAllocation() ||
            !alloc->getType()->getElementType()->isIntegerTy() ) {
          cinput_error( "only pointers to intergers is allowed!" );
        }
        auto alloc_name = "alloc__" + getInstructionLocation( I ).name();
        z3::sort sort = llvm_to_z3_sort(z3.c, alloc->getType()->getElementType());
        p->add_allocated( alloc_name, sort );
        auto v = p->get_allocated( alloc_name );
        points_set_t p_set = { std::make_pair( z3.mk_true(), v ) };
        pointing pointing_( p_set );
        points_to.insert( std::make_pair(alloc,pointing_) );//todo: remove ugly
        assert( !recognized );recognized = true;
      }else{
        cinput_error("unsupported uniary instruction!!" << "\n");
      }
    }

    if( auto bop = llvm::dyn_cast<llvm::BinaryOperator>(I) ) {
      auto op0 = bop->getOperand( 0 );
      auto op1 = bop->getOperand( 1 );
      z3::expr a = getTerm( op0, m );
      z3::expr b = getTerm( op1, m );
      unsigned op = bop->getOpcode();
      switch( op ) {
      case llvm::Instruction::Add : insert_term_map( I, a+b,     m); break;
      case llvm::Instruction::Sub : insert_term_map( I, a-b,     m); break;
      case llvm::Instruction::Mul : insert_term_map( I, a*b,     m); break;
      case llvm::Instruction::And : insert_term_map( I, a && b,  m); break;
      case llvm::Instruction::Or  : insert_term_map( I, a || b,  m); break;
      case llvm::Instruction::Xor:insert_term_map(I,helpers::_xor(a,b),m);break;
      case llvm::Instruction::SDiv: insert_term_map( I, a/b,     m); break;
      default: {
        const char* opName = bop->getOpcodeName();
        cinput_error("unsupported instruction \"" << opName << "\" occurred!!");
      }
      }
      join_depends( op0, op1, local_map[I] );
      assert( !recognized );recognized = true;
    }

    if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
      llvm::Value* lhs = cmp->getOperand( 0 ),* rhs = cmp->getOperand( 1 );
      z3::expr l = getTerm( lhs, m ), r = getTerm( rhs, m );

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
      join_depends( lhs, rhs, local_map[I] );
      assert( !recognized );recognized = true;
    }
    if( const llvm::SelectInst* sel = llvm::dyn_cast<llvm::SelectInst>(I) ) {
      const llvm::Value* cnd = sel->getCondition();
      const llvm::Value* tr = sel->getTrueValue(),*fl = sel->getFalseValue();
      z3::expr c=getTerm(cnd,m), t=getTerm(tr,m), f=getTerm(fl,m), v=getTerm(I,m);
      block_ssa = ( !c || v == t ) && ( c || v == f );
      //todo: depdency for select needs to be clarified
      std::vector<hb_enc::depends_set> dep_ses;
      dep_ses.push_back( get_depends( cnd ) );
      dep_ses.push_back( get_depends( tr  ) );
      dep_ses.push_back( get_depends( fl  ) );
      hb_enc::join_depends_set( dep_ses, local_map[I] );
      // cinput_error( "select not implemented yet!!");
      assert( !recognized );recognized = true;
    }
    if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
      std::vector<hb_enc::depends_set> dep_ses;
      // assert( conds.size() > 1 ); //todo:if not,review initialization of conds
      unsigned num = phi->getNumIncomingValues();
      if( phi->getType()->isIntegerTy() ) {
        z3::expr phi_cons = z3.mk_false();
        z3::expr ov = getTerm(I,m);

        for ( unsigned i = 0 ; i < num ; i++ ) {
          const bb* prev = phi->getIncomingBlock(i);
          const llvm::Value* v_ = phi->getIncomingValue(i);
          if( exists( ignore_edges, prev ) ) continue;
          z3::expr v = getTerm (v_, m );
          phi_cons = phi_cons || (conds.at(prev) && ov == v);
          hb_enc::depends_set temp;
          hb_enc::pointwise_and( get_depends( v_ ), conds.at(prev), temp );
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
    UNSUPPORTED_INSTRUCTIONS( SwitchInst,  I );
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
    if( auto fence = llvm::dyn_cast<llvm::FenceInst>(I) ) {
      assert( fence->getSynchScope()==llvm::SynchronizationScope::CrossThread);
      // auto loc_name = "fence__" + getInstructionLocation( I ).name();
      auto loc_name = getInstructionLocation( I );
      auto fnce = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                             history, loc_name, hb_enc::event_t::barr,
                             translate_ordering_tags( fence->getOrdering() ) );
      p->add_event( thr_id, fnce );
      p->set_c11_rs_heads( fnce, local_release_heads[b] );
      prev_events = { fnce };
      assert( !recognized );recognized = true;
    }
    if( auto call = llvm::dyn_cast<llvm::CallInst>(I) ) {
      if(llvm::isa<llvm::DbgValueInst>(I) ||llvm::isa<llvm::DbgDeclareInst>(I)){
        // Ignore debug instructions
      }else{
        llvm::Function* fp = call->getCalledFunction();
        unsigned argnum = call->getNumArgOperands();
        auto loc_name = getInstructionLocation( I );
        if( fp != NULL && ( fp->getName() == "_Z5fencev" ) ) {
          // auto loc_name = "fence__" + getInstructionLocation( I ).name();
          auto barr = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                                 history, loc_name, hb_enc::event_t::barr );
          p->add_event( thr_id, barr );
          p->set_c11_rs_heads( barr, local_release_heads[b] );
          prev_events.clear(); prev_events.insert( barr );
        }else if( fp != NULL && ( fp->getName() == "pthread_create" ) &&
                  argnum == 4 ) {
          auto thr_ptr = call->getArgOperand(0);
          auto v = call->getArgOperand(2);
          if( !v->hasName() ) {
            v->getType()->print( llvm::outs() ); llvm::outs() << "\n";
            cinput_error("unnamed call to function pointers is not supported!");
          }
          // auto loc_name = "create__" + getInstructionLocation( I ).name();
          auto barr = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                                 history, loc_name, hb_enc::event_t::barr_b );
          std::string fname = (std::string)v->getName();
          ptr_to_create[thr_ptr] = fname;
          p->add_create( thr_id, barr, fname );
          p->set_c11_rs_heads( barr, local_release_heads[b] );
          prev_events.clear(); prev_events.insert( barr );
          // create
        }else if( fp != NULL && ( fp->getName() == "pthread_join" ) ) {
          // join
          auto val = call->getArgOperand(0);
          auto thr_ptr = llvm::cast<llvm::LoadInst>(val)->getOperand(0);
          auto fname = ptr_to_create.at(thr_ptr);
          ptr_to_create.erase( thr_ptr );
          z3::expr join_cond = z3.get_fresh_bool();
          path_cond = path_cond && join_cond;
          // auto loc_name = "join__" + getInstructionLocation( I ).name();
          auto barr = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                                 history, loc_name, hb_enc::event_t::barr_a );
          p->add_join( thr_id, barr, join_cond, fname);
          p->set_c11_rs_heads( barr, local_release_heads[b] );
          join_conds = join_conds && join_cond;
          prev_events.clear(); prev_events.insert( barr );
        }else{
          if( o.print_input > 0 )
            llvm::outs() << "Unknown function " << fp->getName() << "!\n";
          cinput_error( "Unknown function called");
        }
      }
      assert( !recognized );recognized = true;
    }

    if( auto rmw = llvm::dyn_cast<llvm::AtomicRMWInst>(I) ) {
      auto loc = getInstructionLocation( I );

      const auto addr = rmw->getPointerOperand();
      if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
        auto gv = p->get_global( (std::string)(g->getName()) );
        auto up = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond,
                             history, gv, loc, hb_enc::event_t::u,
                             translate_ordering_tags( rmw->getOrdering()) );
        z3::expr wr_v = getTerm( rmw->getValOperand(), m );
        z3::expr rd_v = getTerm( I, m );
        switch( rmw->getOperation() ) {
        case llvm::AtomicRMWInst::BinOp::Xchg: break;
        case llvm::AtomicRMWInst::BinOp::Add : wr_v = up->rd_v() + wr_v; break;
        case llvm::AtomicRMWInst::BinOp::Sub : wr_v = up->rd_v() - wr_v; break;
          // Unsupported
        case llvm::AtomicRMWInst::BinOp::And : wr_v = up->rd_v() && wr_v; break;
        case llvm::AtomicRMWInst::BinOp::Nand:wr_v= !(up->rd_v() && wr_v);break;
        case llvm::AtomicRMWInst::BinOp::Or  : wr_v = up->rd_v() || wr_v; break;
        case llvm::AtomicRMWInst::BinOp::Xor :
        case llvm::AtomicRMWInst::BinOp::Max :
        case llvm::AtomicRMWInst::BinOp::Min :
        case llvm::AtomicRMWInst::BinOp::UMax:
        case llvm::AtomicRMWInst::BinOp::UMin:
        default:
          cinput_error( "unspported atomic rmw operation");
        }
	block_ssa = block_ssa && (up->rd_v() == rd_v) && (up->wr_v() == wr_v);

        local_map[I].insert( hb_enc::depends( up, z3.mk_true() ) );
        up->set_dependencies( get_depends( rmw->getValOperand() ), get_ctrl(b));
        prev_events = { up };
        p->add_event_with_rs_heads( thr_id, prev_events,local_release_heads[b]);
      }else{ cinput_error( "atomic rmw is not supported on pointers!"); }
      assert( !recognized );recognized = true;
    }
    if( auto xchg = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(I) ) {
      assert( !xchg->isWeak() ); //todo: semantics of weak xchg is not clear!!
      auto loc = getInstructionLocation( I );
      const llvm::Value* addr = xchg->getPointerOperand();
      if( auto g = llvm::dyn_cast<llvm::GlobalVariable>( addr ) ) {
        auto gv = p->get_global( (std::string)(g->getName()) );
        z3::expr cmp_v = getTerm( xchg->getCompareOperand(), m );
        z3::expr new_v = getTerm( xchg->getNewValOperand(), m );
        // exchange fail event
        auto rd=mk_se_ptr( hb_enc, thr_id, prev_events, path_cond, history, gv,
                           loc, hb_enc::event_t::r,
                           translate_ordering_tags(xchg->getFailureOrdering()));
        rd->append_history( rd->rd_v() != cmp_v );
        rd->set_dependencies( get_depends( xchg->getCompareOperand() ),
                              get_ctrl(b) );
        // exchange success event
        auto up=mk_se_ptr( hb_enc, thr_id, prev_events, path_cond, history,gv,
                           loc, hb_enc::event_t::u,
                           translate_ordering_tags(xchg->getSuccessOrdering()));
        up->append_history( up->rd_v() == cmp_v );
        hb_enc::depends_set dep;
        join_depends( xchg->getCompareOperand(), xchg->getNewValOperand(), dep);
        rd->set_dependencies( dep, get_ctrl(b) );
        prev_events = { rd, up };
        p->add_event_with_rs_heads(thr_id, prev_events, local_release_heads[b]);
        // joining success and fail events
        //todo: passed branch_conds seems to be wrong; if error fix it!
        auto blk = mk_se_ptr( hb_enc, thr_id, prev_events, path_cond, history,
                              loc, hb_enc::event_t::block, branch_conds );
        prev_events = { blk };
        p->add_event(thr_id, prev_events);
        block_ssa = block_ssa &&
          (((up->rd_v() ==cmp_v)&&(up->wr_v() == new_v))||(rd->rd_v() !=cmp_v));
        cinput_error( "untested code!! for exchange operation!! check clock variable naming")
      }else{
        cinput_error( "Pointers are not yet supported in atomoic xchg!!");
      }

      assert( !recognized );recognized = true;
    }

    if( !recognized ) cinput_error( "----- instruction failed to recognize!!");
  }
  split_step ss( NULL, 0, 0, join_conds);
  block_to_split_stack.at(b).push_back( ss );
  return block_ssa;
}

void
join_depends_set(const std::map<const bb*,hb_enc::depends_set>& deps,
                 const std::map<const bb*,z3::expr>& conds,
                 hb_enc::depends_set& result ) {
  result.clear();
  for( auto& pr : deps ) {
    const bb* b = pr.first;
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

void build_program::collect_loop_backedges() {
  //todo: llvm::FindFunctionBackedges could have done the job
  auto &LIWP = getAnalysis<llvm::LoopInfoWrapperPass>();
  auto LI = &LIWP.getLoopInfo();
  std::vector<llvm::Loop*> loops,stack;
  for(auto I = LI->rbegin(), E = LI->rend(); I != E; ++I) stack.push_back(*I);
  while( !stack.empty() ) {
    llvm::Loop *L = stack.back();
    stack.pop_back();
    loops.push_back( L );
    // for( auto lp : L ) stack.push_back( lp );
    for(auto I = L->begin(), E = L->end(); I != E; ++I) stack.push_back(*I);
  }
  loop_ignore_edges.clear();
  rev_loop_ignore_edges.clear();
  for( llvm::Loop *L : loops ) {
    auto h = L->getHeader();
    llvm::SmallVector<bb*,10> LoopLatches;
    L->getLoopLatches( LoopLatches );
    for( bb* bb : LoopLatches ) {
      loop_ignore_edges[h].insert( bb );
      rev_loop_ignore_edges[bb].insert(h);
    }
    if( o.print_input > 1 ) {
      L->print( llvm::outs(), 3 );
    }
  }
}

bool build_program::runOnFunction( llvm::Function &f ) {
  collect_loop_backedges();

  std::string name = (std::string)f.getName();
  unsigned thread_id = p->add_thread( name );
  hb_enc::se_set prev_events, final_prev_events;
  if( o.print_input > 0 ) std::cout << "Processing Function: " << name << "\n";

  bb_vec_t bb_vec;
  initBlockCount( f, rev_loop_ignore_edges, bb_vec, block_to_id );
  // ordered_blocks( f, ,  );

  // creating the first event of function; needs to initialize several
  // parameters: start_bit, history, location name,
  z3::expr start_bit = z3.get_fresh_bool();
  std::vector< z3::expr > history = { start_bit };
  hb_enc::source_loc loc( name );
  auto start = mk_se_ptr( hb_enc, thread_id, prev_events, start_bit, history,
                          loc, hb_enc::event_t::barr );
  p->set_start_event( thread_id, start, start_bit );
  if( name == "main" ) {
    // if the function is main then we have to declare that it is the
    // entry and exit point of the full program
    p->create_map[ "main" ] = p->init_loc;
    hb_enc::se_ptr post_loc = p->post_loc;
    auto pr = std::make_pair( post_loc, z3.mk_true() );
    p->join_map.insert( std::make_pair( "main" , pr) );
  }

  prev_events.insert( start );

  std::vector< split_history     > final_histories;
  std::vector< const bb* > final_preds;

  for( const bb* src : bb_vec ) {
  // for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
  //   const bb* src = &(*it);
    std::map<const bb*,hb_enc::depends_set> ctrl_ses;
    std::map< const tara::variable,
              std::map<const bb*,hb_enc::depends_set> > last_rls_ses;
    if( o.print_input > 0 ) {
      std::cout << "=====================================================\n";
      std::cout << "Processing block" << " block__" << thread_id << "__"
                << block_to_id[src] << "\n";
      src->print( llvm::outs() );
    }
    if( src->hasName() && (src->getName() == "dummy")) continue;

    std::vector< split_history > histories; // needs to be ref
    std::vector<const bb*> preds;
    std::map<const hb_enc::se_ptr, z3::expr > branch_conds;
    //iterate over predecessors
    std::set<const bb*> ignore_edges;
    if( exists( loop_ignore_edges, src) ) {
      ignore_edges = loop_ignore_edges.at(src);
    }
    for(auto PI = llvm::pred_begin(src),E = llvm::pred_end(src);PI != E;++PI) {
      // const llvm::BasicBlock *prev = *PI;
      const bb* prev = *PI;
      if( exists( ignore_edges, prev ) ) {
        continue; // ignoring loop back edges
      }

      split_history h = block_to_split_stack[prev];
      z3::expr prev_cond = z3.mk_true();
      if( llvm::isa<llvm::BranchInst>( prev->getTerminator() ) ) {
        // read information about the edge
        z3::expr b = block_to_exit_bit.at(prev);
        unsigned succ_num = PI.getUse().getOperandNo();
        llvm::BranchInst* br  = (llvm::BranchInst*)( prev->getTerminator() );
        assert( br->getOperand(succ_num) == src );

        // compute control dependencies
        hb_enc::depends_set ctrl_temp;
        if( !br->isUnconditional() ) {
          llvm::Value* op = prev->getTerminator()->getOperand(0);
          const auto& ctrl_temp0 = get_depends( op );
          const auto& ctrl_temp1 = get_ctrl(prev);
          hb_enc::join_depends_set( ctrl_temp1 , ctrl_temp0, ctrl_temp );
        }else if( br->isUnconditional() ) {
	  ctrl_temp = get_ctrl( prev );
        }
        ctrl_ses[prev] = ctrl_temp;

        // history extension
        if( br->getNumOperands() >= 2 ) {
          // if br is branch insustruction, extend histroy
          if( succ_num == 1 ) b = !b;
          split_step ss(prev, block_to_id[prev], succ_num, b);
          h.push_back(ss);
          prev_cond = b; //todo: hack needs streamlining
        }
      }
      histories.push_back(h);

      //collect incoming branch conditions
      hb_enc::se_set& prev_trail = block_to_trailing_events.at( prev );
      prev_events.insert( prev_trail.begin(), prev_trail.end() );
      assert( prev_trail.size() < 2 );
      for( auto& prev_e : prev_trail ) {
        if( branch_conds.find( prev_e ) != branch_conds.end() ) {
          // if an event is ancestor in multiple paths
          // todo: due to the block events it is never needed.( depreicate )
          cinput_error( "this code is not expected to be executed!!");
          prev_cond = prev_cond || branch_conds.at(prev_e);
        }
        branch_conds.insert( std::make_pair( prev_e, prev_cond) );
      }
      preds.push_back( prev );

      // relase heads calculation for c11
      if( o.mm == mm_t::c11 ) {
        for( const auto& v : p->globals )
          last_rls_ses[v][ prev ] = local_release_heads[prev][v];
      }
    }
    if( src == &(f.getEntryBlock()) ) {
      // initialize the first block
      split_history h;
      split_step ss( NULL, 0, 0, start_bit ); // todo : second param??
      h.push_back( ss );
      histories.push_back( h );
      branch_conds.insert( std::make_pair( start, z3.mk_true()) );
    }

    // join all the histories incoming from the preds
    split_history h;
    std::map<const bb*, z3::expr> conds;
    z3::expr path_cond = z3.mk_true();
    join_histories( preds, histories, h, path_cond, conds);
    block_to_split_stack[src] = h;

    // compute the control dependencies
    local_ctrl[src].clear();
    if( o.mm == mm_t::rmo )
      join_depends_set( ctrl_ses, conds, local_ctrl[src] );

    // caclulating last release heads
     if( o.mm == mm_t::c11 ) {
       for( const auto& v : p->globals )
         join_depends_set( last_rls_ses[v], conds, local_release_heads[src][v]);
     }

    if( src->hasName() && (src->getName() == "err") ) {
      p->append_property( thread_id, !path_cond );
    }else{
      std::vector<z3::expr> history_exprs;
      split_history_to_exprs( h, history_exprs );
      z3::expr ssa = translateBlock( thread_id, src, prev_events, path_cond,
                                     history_exprs, conds, branch_conds);
      block_to_trailing_events[src] = prev_events;
      if( llvm::isa<llvm::ReturnInst>( src->getTerminator() ) ) {
        final_prev_events.insert( prev_events.begin(), prev_events.end() );
        final_histories.push_back( h );
        final_preds.push_back( src );
      }else if( is_dangling( src, rev_loop_ignore_edges ) ) {
        final_prev_events.insert( prev_events.begin(), prev_events.end() );
        final_histories.push_back( h );
        final_preds.push_back( src );
        llvm::outs() << "A dangling block found!!\n";
        src->print( llvm::outs() );
      }
      p->append_ssa( thread_id, ssa );
      if( o.print_input > 0 ) {
        std::cout << "path cond : "<< path_cond << "\n";
        std::cout << "path ssa  : \n";
        tara::debug_print(ssa );
        std::cout << "\n";
      }
    }
    prev_events.clear();
  }

  // create final event of the thread
  split_history final_h;
  std::map<const bb*, z3::expr> conds;
  z3::expr exit_cond = z3.mk_true();
  std::vector<z3::expr> history_exprs;
  hb_enc::source_loc floc( name+"_final" );
  join_histories( final_preds, final_histories, final_h, exit_cond, conds );
  split_history_to_exprs( final_h, history_exprs );
  auto final = mk_se_ptr( hb_enc, thread_id, final_prev_events, exit_cond,
                          history_exprs, floc, hb_enc::event_t::barr );
  p->set_final_event( thread_id, final, exit_cond );

  //todo: should only print for a single thread!!
  if( o.print_input > 1 ) p->print_dependency( o.out() );

  return false;
}

const char * build_program::getPassName() const {
  return "Constructs tara::program from llvm IR";
}

void build_program::getAnalysisUsage(llvm::AnalysisUsage &au) const {
  au.setPreservesAll();
  //TODO: ...
  // au.addRequired<llvm::Analysis>();
  au.addRequired<llvm::LoopInfoWrapperPass>();
}
