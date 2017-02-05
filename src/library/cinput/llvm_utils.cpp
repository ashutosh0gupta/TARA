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
#include "helpers/z3interf.h"
#include <z3.h>
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
#include "llvm/PassManager.h"
#include "llvm/IR/DebugInfo.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
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


int cinput::readInt( const llvm::ConstantInt* c ) {
  const llvm::APInt& n = c->getUniqueInteger();
  unsigned len = n.getNumWords();
  if( len > 1 ) cinput_error( "long integers not supported!!" );
  const uint64_t *v = n.getRawData();
  return *v;
}

bool cinput::is_llvm_false( llvm::Value* v ) {
  if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(v) ) {
    // unsigned bw = c->getBitWidth();
    // if( bw == 1 ) {
      int i = readInt( c );
      if( i == 0 ) return true;
    // }
  }
  return false;
}

bool cinput::is_llvm_true( llvm::Value* v ) {
  if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(v) ) {
    unsigned bw = c->getBitWidth();
    if( bw == 1 ) {
      int i = readInt( c );
      if( i == 1 ) return true;
    }
  }
  return false;
}

z3::sort cinput::llvm_to_z3_sort( z3::context& c, llvm::Type* t ) {
  if( t->isIntegerTy() ) {
    if( t->isIntegerTy( 32 ) ) return c.int_sort();
    if( t->isIntegerTy( 64 ) ) return c.int_sort();
    if( t->isIntegerTy( 8 ) ) return c.bool_sort();
  }
  cinput_error( "only int and bool sorts are supported");
  // return c.bv_sort(32); // needs to be added
  // return c.bv_sort(16);
  // return c.bv_sort(64);
  // return c.bool_sort();
  // return c.real_sort();
  return c.int_sort(); // dummy return
}

std::string  cinput::getInstructionLocationName(const llvm::Instruction* I ) {
  const llvm::DebugLoc d = I->getDebugLoc();
  unsigned line = d.getLine();
  unsigned col  = d.getCol();

  return "_l"+std::to_string(line)+"_c"+std::to_string(col);
}

void cinput::initBlockCount( llvm::Function &f,
                     std::map<const llvm::BasicBlock*, unsigned>& block_to_id) {
  unsigned l = 0;
  block_to_id.clear();
  for( auto it  = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* b = &(*it);
    block_to_id[b] = l++;
  }
}

void cinput::removeBranchingOnPHINode( llvm::BranchInst *branch ) {
    if( branch->isUnconditional() ) return;
    llvm::Value* cond = branch->getCondition();
    if( llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(cond) ) {
      llvm::BasicBlock* phiBlock = branch->getParent();
      llvm::BasicBlock* phiDstTrue = branch->getSuccessor(0);
      llvm::BasicBlock* phiDstFalse = branch->getSuccessor(1);
      if( phiBlock->size() == 2 && cond == (phiBlock->getInstList()).begin()) {
        unsigned num = phi->getNumIncomingValues();
        for( unsigned i = 0; i <= num - 2; i++ ) {
        llvm::Value* val0      = phi->getIncomingValue(i);
        llvm::BasicBlock* src0 = phi->getIncomingBlock(i);
        llvm::BranchInst* br0  = (llvm::BranchInst*)(src0->getTerminator());
        unsigned br0_branch_idx = ( br0->getSuccessor(0) ==  phiBlock ) ? 0 : 1;
        if( llvm::ConstantInt* b = llvm::dyn_cast<llvm::ConstantInt>(val0) ) {
          assert( b->getType()->isIntegerTy(1) );
          llvm::BasicBlock* newDst = b->getZExtValue() ?phiDstTrue:phiDstFalse;
          br0->setSuccessor( br0_branch_idx, newDst );
        }else{ cinput_error("unseen case!");}
        }
        llvm::Value*      val1 = phi->getIncomingValue(num-1);
        llvm::BasicBlock* src1 = phi->getIncomingBlock(num-1);
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
      // bb->print( llvm::outs() );
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
            llvm::Value * arg0 = call->getArgOperand(0);
            if( !llvm::isa<llvm::Constant>(arg0) ) {
              auto arg_iit = iit;
              llvm::Instruction* arg = --arg_iit;
              if( arg != arg0 )
                cinput_error( "previous instruction not passed!!\n" );
            }
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
      llvm::BasicBlock* elseBlock = isAssume[i] ? dum : err;
      llvm::Instruction* cmp = NULL;
      llvm::BranchInst *branch;
      if( llvm::Instruction* c = llvm::dyn_cast<llvm::Instruction>(args[i]) ) {
        cmp = c;
        llvm::BasicBlock* tail = llvm::SplitBlock( head, splitIs[i], this );
        branch = llvm::BranchInst::Create( tail, elseBlock, cmp );
        llvm::ReplaceInstWithInst( head->getTerminator(), branch );
      } else if( is_llvm_false(args[i]) ) { // jump to else block
        llvm::BasicBlock* _tail = llvm::SplitBlock( head, splitIs[i], this );
        //todo: remove tail block and any other block that is unreachable now
        branch = llvm::BranchInst::Create( elseBlock );
        llvm::ReplaceInstWithInst( head->getTerminator(), branch );
      } else if( is_llvm_true(args[i]) ) { // in case of true
        // do nothing and delete the call
      }else{
        head->print( llvm::outs() );
        cinput_error( "instruction expected here!!\n" );
      }
      calls[i]->eraseFromParent();
      if( cmp != NULL && llvm::isa<llvm::PHINode>(cmp) ) {
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
