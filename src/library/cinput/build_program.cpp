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


//----------------------------------------------------------------------------
// LLVM utils

#define UNSUPPORTED_INSTRUCTIONS( InstTYPE, Inst )                       \
  if(llvm::isa<llvm::InstTYPE>(Inst) ) {                                 \
     cinput_error( "Unsupported instruction!!");                         \
  }


int readInt( const llvm::ConstantInt* c ) {
  const llvm::APInt& n = c->getUniqueInteger();
  unsigned len = n.getNumWords();
  if( len > 1 ) cinput_error( "long integers not supported!!" );
  const uint64_t *v = n.getRawData();
  return *v;
}


std::string getInstructionLocationName(const llvm::Instruction* I ) {
  const llvm::DebugLoc d = I->getDebugLoc();
  unsigned line = d.getLine();
  unsigned col  = d.getCol();

  return "_l"+std::to_string(line)+"_c"+std::to_string(col);
}

void initBlockCount( llvm::Function &f,
                     std::map<llvm::BasicBlock*, unsigned>& block_to_id) {
  unsigned l = 0;
  block_to_id.clear();
  for( auto it  = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* b = &(*it);
    // auto new_l = program->createFreshLocation(threadId);
    block_to_id[b] = l++;
    // if( b->getName() == "err" ) {
    //   program->setErrorLocation( threadId, new_l );
    // }
  }
    // llvm::BasicBlock& b  = f.getEntryBlock();
    // unsigned entryLoc = getBlockCount( &b );
    // program->setStartLocation( threadId, entryLoc );
    // auto finalLoc = program->createFreshLocation(threadId);
    // program->setFinalLocation( threadId, finalLoc );
}

void removeBranchingOnPHINode( llvm::BranchInst *branch ) {
    if( branch->isUnconditional() ) return;
    llvm::Value* cond = branch->getCondition();
    if( llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(cond) ) {
      llvm::BasicBlock* phiBlock = branch->getParent();
      llvm::BasicBlock* phiDstTrue = branch->getSuccessor(0);
      llvm::BasicBlock* phiDstFalse = branch->getSuccessor(1);
      if( phiBlock->size() == 2 && cond == (phiBlock->getInstList()).begin() &&
          phi->getNumIncomingValues() == 2 ) {
        llvm::Value* val0      = phi->getIncomingValue(0);
        llvm::BasicBlock* src0 = phi->getIncomingBlock(0);
        llvm::BranchInst* br0  = (llvm::BranchInst*)(src0->getTerminator());
        unsigned br0_branch_idx = ( br0->getSuccessor(0) ==  phiBlock ) ? 0 : 1;
        if( llvm::ConstantInt* b = llvm::dyn_cast<llvm::ConstantInt>(val0) ) {
          assert( b->getType()->isIntegerTy(1) );
          llvm::BasicBlock* newDst = b->getZExtValue() ?phiDstTrue:phiDstFalse;
          br0->setSuccessor( br0_branch_idx, newDst );
        }else{ cinput_error("unseen case!");}
        llvm::Value*      val1 = phi->getIncomingValue(1);
        llvm::BasicBlock* src1 = phi->getIncomingBlock(1);
        llvm::BranchInst* br1  = (llvm::BranchInst*)(src1->getTerminator());
        llvm::Instruction* cmp1 = llvm::dyn_cast<llvm::Instruction>(val1);
        assert( cmp1 );
        assert( br1->isUnconditional() );
        if( cmp1 != NULL && br1->isUnconditional() ) {
          llvm::BranchInst *newBr =
            llvm::BranchInst::Create( phiDstTrue, phiDstFalse, cmp1);
          llvm::ReplaceInstWithInst( br1, newBr );
          // TODO: memory leaked here? what happend to br1 was it deleted??
          llvm::DeleteDeadBlock( phiBlock );
          removeBranchingOnPHINode( newBr );
        }else{ cinput_error("unseen case, not known how to handle!"); }
      }else{ cinput_error("unseen case, not known how to handle!"); }
    }
  }

//----------------------------------------------------------------------------
// Splitting for assume pass
  char SplitAtAssumePass::ID = 0;
  bool SplitAtAssumePass::runOnFunction( llvm::Function &f ) {
    llvm::LLVMContext &C = f.getContext();
    llvm::BasicBlock *dum = llvm::BasicBlock::Create( C, "dummy", &f, f.end());
    new llvm::UnreachableInst(C, dum);
    llvm::BasicBlock *err = llvm::BasicBlock::Create( C, "err", &f, f.end() );
    new llvm::UnreachableInst(C, err);

    std::vector<llvm::BasicBlock*> splitBBs;
    std::vector<llvm::CallInst*> calls;
    std::vector<llvm::Value*> args;
    std::vector<llvm::Instruction*> splitIs;
    std::vector<bool> isAssume;

     //collect calls to assume statements
    auto bit = f.begin(), end = f.end();
    for( ; bit != end; ++bit ) {
      llvm::BasicBlock* bb = bit;
      auto iit = bb->begin(), iend = bb->end();
      size_t bb_sz = bb->size();
      for( unsigned i = 0; iit != iend; ++iit ) {
	i++;
        llvm::Instruction* I = iit;
        if( llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(I) ) {
          llvm::Function* fp = call->getCalledFunction();
	  if( fp != NULL &&
              ( fp->getName() == "_Z6assumeb" ||
                fp->getName() == "_Z6assertb" ) &&
              i < bb_sz ) {
	    splitBBs.push_back( bb );
            auto arg_iit = iit;
	    llvm::Instruction* arg = --arg_iit;
            llvm::Value * arg0 = call->getArgOperand(0);
	    if( arg != arg0 ) std::cerr << "last out not passed!!";
            calls.push_back( call );
            args.push_back( arg0 );
	    auto local_iit = iit;
            splitIs.push_back( ++local_iit );
            isAssume.push_back( (fp->getName() == "_Z6assumeb") );
	  }
        }
      }
    }
    // Reverse order splitting for the case if there are more than
    // one assumes in one basic block
    for( unsigned i = splitBBs.size(); i != 0 ; ) { i--;
      llvm::BasicBlock* head = splitBBs[i];
      llvm::BasicBlock* tail = llvm::SplitBlock( head, splitIs[i], this );
      llvm::Instruction* cmp;
      if( llvm::Instruction* c = llvm::dyn_cast<llvm::Instruction>(args[i]) ) {
        cmp = c;
      }else{ std::cerr << "instruction expected here!!"; }
      llvm::BasicBlock* elseBlock = isAssume[i] ? dum : err;
      llvm::BranchInst *branch =llvm::BranchInst::Create( tail, elseBlock, cmp);
      llvm::ReplaceInstWithInst( head->getTerminator(), branch );
      calls[i]->eraseFromParent();
      if( llvm::isa<llvm::PHINode>(cmp) ) {
	removeBranchingOnPHINode( branch );
      }
    }
    return false;
  }

  void SplitAtAssumePass::getAnalysisUsage(llvm::AnalysisUsage &au) const {
    // it modifies blocks therefore may not preserve anything
    // au.setPreservesAll();
    //TODO: ...
    // au.addRequired<llvm::Analysis>();
  }
//----------------------------------------------------------------------------
// code for input object generation

char build_program::ID = 0;
void
build_program::join_histories( const std::vector< llvm::BasicBlock* > preds,
                               const std::vector< split_history >& hs,
                               split_history& h,
                               std::map< const llvm::BasicBlock*, z3::expr >& conds) {
  h.clear();
  if(hs.size() == 0 ) return;
  if(hs.size() == 1 ) {
    h = hs[0];
    return;
  }
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
      throw cinput_exception( "histories can not be subset of each other!!" );
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


z3::expr build_program::getTerm( const llvm::Value* op ,ValueExprMap& m ) {
      if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
        unsigned bw = c->getBitWidth();
        if(bw > 1) {
          fresh_int();
        }else if(bw == 1) {
          fresh_bool();
        }else
          std::cerr << "unrecognized constant!";
      }else if( auto c = llvm::dyn_cast<llvm::ConstantPointerNull>(op) ) {
        // }else if( LLCAST( llvm::ConstantPointerNull, c, op) ) {
        return z3.c.int_val(0);
      }else if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
        std::cerr << "un recognized constant!";
        //     // int i = readInt(c);
        //     // return eHandler->mkIntVal( i );
      }else if( auto* c = llvm::dyn_cast<llvm::ConstantFP>(op) ) {
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
      }else if( auto c = llvm::isa<llvm::ConstantVector>(op) ) {
        const llvm::VectorType* n = c->getType();
        cinput_error( "vector constant not implemented!!" );
      }else{
        auto it = m.find( op );
        if( it == m.end() ) {
          llvm::outs() << "\n";
          std::cerr << "local term not found!";
        }
        return it->second;
      }
}

z3::expr build_program::translateBlock( unsigned thr_id,
                                        const llvm::BasicBlock* b,
                                        hb_enc::se_set& prev_events,
                               std::map<const llvm::BasicBlock*,z3::expr>& conds) {
  assert(b);
  z3::expr block_ssa = z3.mk_true();
  // std::vector<typename EHandler::expr> blockTerms;
  // //iterate over instructions
  for( const llvm::Instruction& Iobj : b->getInstList() ) {
    const llvm::Instruction* I = &(Iobj);
    assert( I );
    bool recognized = false;
    if( auto wr = llvm::dyn_cast<llvm::StoreInst>(I) ) {
      llvm::Value* v = wr->getOperand(0);
      llvm::GlobalVariable* g = (llvm::GlobalVariable*)wr->getOperand(1);
      cssa::variable gv = p->get_global( (std::string)(g->getName()) );
      std::string loc_name = getInstructionLocationName( I );
      cssa::variable ssa_var = gv + "#" + loc_name;
      auto wr_e = mk_se_ptr( z3.c, hb_encoding, thr_id, prev_events,
                             ssa_var, gv, loc_name, hb_enc::event_kind_t::w );
      prev_events.clear(); prev_events.insert( wr_e );
      block_ssa = block_ssa && ( ssa_var == getTerm( v, m ));
      p->add_event( thr_id, wr_e );
    }
    if( auto bop = llvm::dyn_cast<llvm::BinaryOperator>(I) ) {
      llvm::Value* op0 = bop->getOperand( 0 );
      llvm::Value* op1 = bop->getOperand( 1 );
      z3::expr a = getTerm( op0, m );
      z3::expr b = getTerm( op1, m );
      unsigned op = bop->getOpcode();
      switch( op ) {
      case llvm::Instruction::Add : insert_term_map( I, a+b, m); break;
      case llvm::Instruction::Sub : insert_term_map( I, a-b, m); break;
      case llvm::Instruction::Mul : insert_term_map( I, a*b, m); break;
      case llvm::Instruction::Xor : insert_term_map( I, a xor b, m); break;
      case llvm::Instruction::SDiv: insert_term_map( I, a/b, m); break;
      default: {
        const char* opName = bop->getOpcodeName();
        cinput_error("unsupported instruction " << opName << " occurred!!");
      }
      }
      assert( !recognized );recognized = true;
    }
    if( llvm::isa<llvm::UnaryInstruction>(I) ) {
      if( auto load = llvm::dyn_cast<llvm::LoadInst>(I) ) {
        llvm::GlobalVariable* g = (llvm::GlobalVariable*)load->getOperand(0);
        cssa::variable gv = p->get_global( (std::string)(g->getName()) );
        std::string loc_name = getInstructionLocationName( I );
        cssa::variable ssa_var = gv + "#" + loc_name;
        auto rd_e = mk_se_ptr( z3.c, hb_encoding, thr_id, prev_events,
                               ssa_var, gv, loc_name, hb_enc::event_kind_t::r );
        prev_events.clear(); prev_events.insert( rd_e );
        z3::expr l_v = getTerm( I, m);
        block_ssa = block_ssa && ( ssa_var == l_v);
        assert( !recognized );recognized = true;
      }else{
        cinput_error("unsupported uniary instruction!!" << "\n");
      }
    }
    if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
      llvm::Value* lhs = cmp->getOperand( 0 );
      llvm::Value* rhs = cmp->getOperand( 1 );
      z3::expr l = getTerm( lhs, m );
      z3::expr r = getTerm( rhs, m );
      llvm::CmpInst::Predicate pred = cmp->getPredicate();

      switch( pred ) {
      case llvm::CmpInst::ICMP_EQ : insert_term_map( I, l==r, m ); break;
      case llvm::CmpInst::ICMP_NE : insert_term_map( I, l!=r, m ); break;
      case llvm::CmpInst::ICMP_UGT : insert_term_map( I, l>r, m ); break;
      case llvm::CmpInst::ICMP_UGE : insert_term_map( I, l>=r, m ); break;
      case llvm::CmpInst::ICMP_ULT : insert_term_map( I, l<r, m ); break;
      case llvm::CmpInst::ICMP_ULE : insert_term_map( I, l<=r, m ); break;
      case llvm::CmpInst::ICMP_SGT : insert_term_map( I, l>r, m ); break;
      case llvm::CmpInst::ICMP_SGE : insert_term_map( I, l>=r, m ); break;
      case llvm::CmpInst::ICMP_SLT : insert_term_map( I, l<r, m ); break;
      case llvm::CmpInst::ICMP_SLE : insert_term_map( I, l<=r, m ); break;
      default: {
        cinput_error( "unsupported predicate in compare " << pred << "!!");
      }
     }
      assert( !recognized );recognized = true;
    }
    if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
      
      // term = getPhiMap( phi, m);
      // assert( !recognized );recognized = true;
      //block_ssa = block_ssa && ( ssa_var = getphiMap( phi, m ) );
      unsigned num = phi->getNumIncomingValues();
      for ( unsigned i = 0 ; i < num ; i++ ) {
        const llvm::BasicBlock* b = phi->getIncomingBlock(i);
        const llvm::Value* v_ = phi->getIncomingValue(i);
	z3::expr v = getTerm (v_, m );
	conds.at(b) && getTerm(I,m) == v;
      }

    }

    if( auto ret = llvm::dyn_cast<llvm::ReturnInst>(I) ) {
      llvm::Value* v = ret->getReturnValue();
      if( v ) {
  //       typename EHandler::expr retTerm = getTerm( v, m );
  //       //todo: use retTerm somewhere
      }
      if( o.print_input > 0 ) std::cerr << "return value ignored!!";
      assert( !recognized );recognized = true;
    }
    if( llvm::isa<llvm::UnreachableInst>(I) ) {
      if( o.print_input > 0 ) std::cerr << "unreachable instruction ignored!!";
      assert( !recognized );recognized = true;
    }

    // UNSUPPORTED_INSTRUCTIONS( ReturnInst,      I );
    UNSUPPORTED_INSTRUCTIONS( InvokeInst,      I );
    UNSUPPORTED_INSTRUCTIONS( IndirectBrInst,  I );
    // UNSUPPORTED_INSTRUCTIONS( UnreachableInst, I );

    if( auto br = llvm::dyn_cast<llvm::BranchInst>(I) ) {
      if(  br->isConditional() ) {
        block_to_exit_bit.at(b) = getTerm( br->getCondition(), m);
      }else{
        block_to_exit_bit.at(b) = fresh_bool();
      }
      assert( !recognized );recognized = true;
    }
    UNSUPPORTED_INSTRUCTIONS( SwitchInst,  I );
    // if( llvm::isa<llvm::SwitchInst>(I) ) {
    //   cinput_error( "switch statement not supported yet!!");
    //   assert( !recognized );recognized = true;
    // }
    if( auto call = llvm::dyn_cast<llvm::CallInst>(I) ) {
      if( llvm::isa<llvm::DbgValueInst>(I) ) {
        // Ignore debug instructions
      }else{
        llvm::Function* fp = call->getCalledFunction();
        if( fp != NULL && ( fp->getName() == "_Z5fencev" ) ) {
          std::string loc_name = "fence__" + getInstructionLocationName( I );
          auto barr = mk_se_ptr( z3.c, hb_encoding, thr_id, prev_events,
                                 loc_name, hb_enc::event_kind_t::barr );
          p->add_event( thr_id, barr );
          prev_events.clear(); prev_events.insert( barr );
        }else{
          cinput_error( "Unknown function called");
          if( o.print_input > 0 ) std::cerr << "Unknown function called";
        }
      }
      assert( !recognized );recognized = true;
    }
    if( !recognized ) cinput_error( "----- failed to recognize!!");
  }
  return block_ssa;
}

bool build_program::runOnFunction( llvm::Function &f ) {
  std::string name = (std::string)f.getName();
  unsigned thread_id = p->add_thread( name );

  initBlockCount( f, block_to_id );


  for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* src = &(*it);
    if( o.print_input > 0 ) src->print( llvm::outs() );

    std::vector< split_history > histories; // needs to be ref
    std::vector<llvm::BasicBlock*> preds;
    hb_enc::se_set prev_events;
    for(auto PI = llvm::pred_begin(src),E = llvm::pred_end(src);PI != E;++PI){
      llvm::BasicBlock *prev = *PI;
      split_history h = block_to_split_stack[prev];
      if( llvm::isa<llvm::BranchInst>( prev->getTerminator() ) ) {
        z3::expr& b = block_to_exit_bit.at(prev);
        // llvm::TerminatorInst *term= prev->getTerminator();
        unsigned succ_num = PI.getOperandNo();
        if( succ_num == 1 ) b = !b;
        split_step ss(prev, block_to_id[prev], succ_num, b);
        h.push_back(ss);
      }
      histories.push_back(h);
      hb_enc::se_set& prev_trail = block_to_trailing_events.at(prev);
      prev_events.insert( prev_trail.begin(), prev_trail.end() );
      preds.push_back( prev );
    }
    split_history h;
    std::map<const llvm::BasicBlock*, z3::expr> conds;
    join_histories( preds, histories, h, conds);
    block_to_split_stack[src] = h;

    z3::expr ssa = translateBlock( thread_id, src, prev_events, conds);
    block_to_trailing_events[src] = prev_events;
    p->append_ssa( ssa );

  //     typename EHandler::expr e = translateBlock( src, 0, exprMap );

  //   unsigned srcLoc = getBlockCount( src );
  //   llvm::TerminatorInst* c= (*it).getTerminator();
  //   unsigned num = c->getNumSuccessors();
  //   if( num == 0 ) {
  //     typename EHandler::expr e = translateBlock( src, 0, exprMap );
  //     auto dstLoc = program->getFinalLocation( threadId );
  //     program->addBlock( threadId, srcLoc, dstLoc, e );
  //   }else{
  //     for( unsigned i=0; i < num; ++i ) {
  //       // ith branch may be tranlate differently
  //       typename EHandler::expr e = translateBlock( src, i, exprMap );
  //       llvm::BasicBlock* dst = c->getSuccessor(i);
  //       unsigned dstLoc = getBlockCount( dst );
  //       program->addBlock( threadId, srcLoc, dstLoc, e );
  //     }
  //   }
  }
  // applyPendingInsertEdges( threadId );

  return false;
}
