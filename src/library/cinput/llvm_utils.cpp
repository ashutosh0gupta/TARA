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
#include "helpers/graph_utils.h"

// #include "helpers/z3interf.h"
#include <z3.h>
#pragma GCC optimize ("no-rtti")

using namespace tara;
using namespace tara::cinput;
using namespace tara::helpers;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// pragam'ed to avoid warnings due to llvm included files

#include "llvm/IRReader/IRReader.h"

#include "llvm/IR/InstIterator.h"

#include "llvm/LinkAllPasses.h"
#include "llvm/Pass.h"
// #include "llvm/PassManager.h" //3.6
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
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
#include "llvm/Support/CommandLine.h"

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
  llvm::outs() << "ty :" ; t->print( llvm::outs() ); llvm::outs() << "\n";
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
  static std::set< std::pair<unsigned, unsigned> > seen_before;
  static unsigned unknown_location_counter = 0;
  if( d ) {
    unsigned line = d.getLine();
    unsigned col  = d.getCol();
    std::string l_name = "_l" + std::to_string(line) + "_c" + std::to_string(col);
    auto line_col = std::make_pair(line, col);
    if( exists( seen_before, line_col ) ) {
      return l_name + "_u" + std::to_string(unknown_location_counter++);
    }else{
      seen_before.insert( line_col );
    }
    return l_name;
    // return "_l" + std::to_string(line) + "_c" + std::to_string(col);
  }else{
    return "_u_" + std::to_string(unknown_location_counter++);
  }
}

hb_enc::source_loc cinput::getInstructionLocation(const llvm::Instruction* I ) {
  const llvm::DebugLoc d = I->getDebugLoc();
  hb_enc::source_loc l;
  if( d ) {
    l.line = d.getLine();
    l.col  = d.getCol();
    //l.file //todo: get filename
  }
  return l;
}

hb_enc::source_loc cinput::getBlockLocation(const bb* b ) {
  auto I = b->getFirstNonPHIOrDbg();
  return getInstructionLocation(I);
}

void cinput::initBlockCount( llvm::Function &f,
                     std::map<const bb*, unsigned>& block_to_id) {
  unsigned l = 0;
  block_to_id.clear();
  for( auto it  = f.begin(), end = f.end(); it != end; it++ ) {
    bb* b = &(*it);
    block_to_id[b] = l++;
  }
}

void cinput::setLLVMConfigViaCommandLineOptions( std::string strs ) {
  std::string n = "test";
  // char str[2][30];
  // strcpy( str[0], "test" );
  // strcpy( str[1], "-debug-pass" );
  const char* array[2];
  array[0] = n.c_str();
  array[1] = strs.c_str();
  // str[0] = "test";
  // str[1] = "-debug-pass";
  llvm::cl::ParseCommandLineOptions( 2, array );
}

void cinput::removeBranchingOnPHINode( llvm::BranchInst *branch ) {
    if( branch->isUnconditional() ) return;
    auto cond = branch->getCondition();
    if( llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(cond) ) {
      bb* phiBlock = branch->getParent();
      bb* phiDstTrue = branch->getSuccessor(0);
      bb* phiDstFalse = branch->getSuccessor(1);
      llvm::Instruction* first_inst = &*(phiBlock->getInstList()).begin();//3.8
      if( phiBlock->size() == 2 && cond == first_inst ) {
        unsigned num = phi->getNumIncomingValues();
        for( unsigned i = 0; i <= num - 2; i++ ) {
        llvm::Value* val0      = phi->getIncomingValue(i);
        bb* src0 = phi->getIncomingBlock(i);
        llvm::BranchInst* br0  = (llvm::BranchInst*)(src0->getTerminator());
        unsigned br0_branch_idx = ( br0->getSuccessor(0) ==  phiBlock ) ? 0 : 1;
        if( llvm::ConstantInt* b = llvm::dyn_cast<llvm::ConstantInt>(val0) ) {
          assert( b->getType()->isIntegerTy(1) );
          bb* newDst = b->getZExtValue() ?phiDstTrue:phiDstFalse;
          br0->setSuccessor( br0_branch_idx, newDst );
        }else{ cinput_error("unseen case!");}
        }
        llvm::Value*      val1 = phi->getIncomingValue(num-1);
        bb* src1 = phi->getIncomingBlock(num-1);
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

void cinput::dump_dot_module( boost::filesystem::path& dump_path,
                              std::unique_ptr<llvm::Module>& module ) {
  auto c_path = boost::filesystem::current_path();
  current_path( dump_path );
  llvm::legacy::PassManager passMan;
  passMan.add( llvm::createCFGPrinterPass() );
  passMan.run( *module.get() );
  current_path( c_path );
}

class bb_succ_iter : public llvm::succ_const_iterator {
public:
  bb_succ_iter( llvm::succ_const_iterator begin_,
                llvm::succ_const_iterator end_,
                bb_set_t& back_edges ) :
    llvm::succ_const_iterator( begin_ ), end(end_), b_edges( back_edges ) {
    llvm::succ_const_iterator& it = (llvm::succ_const_iterator&)*this;
    while( it != end && helpers::exists( b_edges, (const bb*)*it) ) ++it;
  };

  bb_succ_iter( llvm::succ_const_iterator begin_,
                llvm::succ_const_iterator end_ ) :
    llvm::succ_const_iterator( begin_ ), end(end_) {};

  bb_succ_iter( llvm::succ_const_iterator end_ ) :
    llvm::succ_const_iterator( end_ ), end( end_ ) {};

  llvm::succ_const_iterator end;
  bb_set_t b_edges;
  bb_succ_iter& operator++() {
    llvm::succ_const_iterator& it = (llvm::succ_const_iterator&)*this;
    do{
      ++it;
    }while( it != end && helpers::exists( b_edges, (const bb*)*it) );
    // std::cout << b_edges.size() << "\n";
    return *this;
  }
};



void cinput::ordered_blocks( const llvm::Function &F,
                             std::map<const bb*,bb_set_t> bedges,
                             std::vector<const bb*>& bs ) {

  auto f = [&bedges](const bb* b) {
    if( exists( bedges, b ) ) {
      return bb_succ_iter( llvm::succ_begin(b), llvm::succ_end(b),bedges.at(b));
    }else{
      return bb_succ_iter( llvm::succ_begin(b), llvm::succ_end(b));
    }
  };

  auto e = [](const bb* b) { return bb_succ_iter( llvm::succ_end(b) ); };

  const bb* h = &F.getEntryBlock();
  topological_sort<const bb*, bb_succ_iter>( h, f, e, bs );
}


//----------------------------------------------------------------------------
// Splitting for assume pass
char SplitAtAssumePass::ID = 0;
bool SplitAtAssumePass::runOnFunction( llvm::Function &f ) {
    llvm::LLVMContext &C = f.getContext();
    llvm::BasicBlock *dum = llvm::BasicBlock::Create( C, "dummy", &f, &*f.end());
    new llvm::UnreachableInst(C, dum);
    llvm::BasicBlock *err = llvm::BasicBlock::Create( C, "err", &f, &*f.end() );
    new llvm::UnreachableInst(C, err);

    std::vector<bb*> splitBBs;
    std::vector<llvm::CallInst*> calls;
    std::vector<llvm::Value*> args;
    std::vector<llvm::Instruction*> splitIs;
    std::vector<bool> isAssume;

     //collect calls to assume statements
    auto bit = f.begin(), end = f.end();
    for( ; bit != end; ++bit ) {
      bb* bb = &*bit;
      // bb->print( llvm::outs() );
      auto iit = bb->begin(), iend = bb->end();
      size_t bb_sz = bb->size();
      for( unsigned i = 0; iit != iend; ++iit ) {
	i++;
        llvm::Instruction* I = &*iit;
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
              llvm::Instruction* arg = &*(--arg_iit);
              if( arg != arg0 )
                cinput_error( "previous instruction not passed!!\n" );
            }
            calls.push_back( call );
            args.push_back( arg0 );
	    auto local_iit = iit;
            splitIs.push_back( &*(++local_iit) );
            isAssume.push_back( (fp->getName() == "_Z6assumeb") );
	  }
        }
      }
    }
    // Reverse order splitting for the case if there are more than
    // one assumes in one basic block
    for( unsigned i = splitBBs.size(); i != 0 ; ) { i--;
      bb* head = splitBBs[i];
      bb* elseBlock = isAssume[i] ? dum : err;
      llvm::Instruction* cmp = NULL;
      llvm::BranchInst *branch;
      if( llvm::Instruction* c = llvm::dyn_cast<llvm::Instruction>(args[i]) ) {
        cmp = c;
        bb* tail = llvm::SplitBlock( head, splitIs[i] ); //3.8
        // bb* tail = llvm::SplitBlock( head, splitIs[i], this ); // 3.6
        branch = llvm::BranchInst::Create( tail, elseBlock, cmp );
        llvm::ReplaceInstWithInst( head->getTerminator(), branch );
      } else if( is_llvm_false(args[i]) ) { // jump to else block
        // bb* _tail =
          llvm::SplitBlock( head, splitIs[i] ); //3.8
        // bb* _tail = llvm::SplitBlock( head, splitIs[i], this ); //3.6
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

const char * SplitAtAssumePass::getPassName() const {
  return "Split blocks at assume/assert statements";
}

void SplitAtAssumePass::getAnalysisUsage(llvm::AnalysisUsage &au) const {
  // it modifies blocks therefore may not preserve anything
  // au.setPreservesAll();
  //TODO: ...
  // au.addRequired<llvm::Analysis>();
}
