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

#ifndef TARA_CINPUT_BUILD_PROGRAM_H
#define TARA_CINPUT_BUILD_PROGRAM_H

#include <map>
#include "cinput/cinput.h"
#include "helpers/z3interf.h"
#include "api/options.h"
#include "helpers/helpers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/Support/raw_ostream.h"
#pragma GCC diagnostic pop

namespace tara {
namespace cinput {

  class split_step {
  public:
    split_step( llvm::BasicBlock* b_,
                unsigned block_id_,
                unsigned succ_num_,
                z3::expr e_):
      b(b_), block_id(block_id_), succ_num(succ_num_), cond(e_) {}
    llvm::BasicBlock* b;
    unsigned block_id;
    unsigned succ_num;
    z3::expr cond;
    // friend bool operator==(const split_step& step1, const split_step& step2);
  };

  typedef std::vector<split_step> split_history;

  class SplitAtAssumePass : public llvm::FunctionPass {
  public:
    static char ID;
    SplitAtAssumePass() : llvm::FunctionPass(ID) {}
    virtual bool runOnFunction( llvm::Function &f );
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;
  };


  class build_program : public llvm::FunctionPass {

  public:
    typedef std::map< const llvm::Value*, z3::expr > ValueExprMap;
    ValueExprMap m;
    std::map< const llvm::Value*, cssa::variable > localVars;
    static char ID;

    //SimpleMultiThreadedProgram<z3:expr>::location_id_type program_location_id_t;
    // SimpleMultiThreadedProgram<z3:expr>::thread_id_type thread_id_t;

  private:
    helpers::z3interf& z3;
    api::options& o;
    // Cfrontend::Config& config;
    program* p;
    unsigned inst_counter = 0;
    unsigned thread_count = 0;
    ValueExprMap valMap;
    std::map< llvm::BasicBlock*, unsigned> block_to_id;
    std::map< llvm::BasicBlock*, split_history > block_to_split_stack;
    std::map< llvm::BasicBlock*, z3::expr > block_to_exit_bit;
    std::map< llvm::BasicBlock*, hb_enc::se_set>  block_to_trailing_events;

    //
    std::map< llvm::BasicBlock*, z3::expr > block_to_path_con;

    void join_histories( const std::vector<llvm::BasicBlock*> preds,
                         const std::vector<split_history>& hs,
                         split_history& h,
                         std::map<llvm::BasicBlock*,z3::expr>& conds
                         );
  public:
    build_program( helpers::z3interf& z3_,
                   api::options& o_,
                   program* program_ )
    : llvm::FunctionPass(ID)
    , z3(z3_)
    , o(o_)
    , p( program_ )
    , thread_count( 0 )
{}

    virtual bool runOnFunction( llvm::Function & );

    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {
      au.setPreservesAll();
      //TODO: ...
      // au.addRequired<llvm::Analysis>();
    }

  private:
    // EHandler* eHandler;
    // SimpleMultiThreadedProgram<z3:expr>* program;
    // std::map< const llvm::BasicBlock*, program_location_id_t > numBlocks;
    // std::vector<unsigned> pendingSrc;
    // std::vector<unsigned> pendingDst;
    // std::vector<z3:expr> pendingTerms;

    //private functions
    // program_location_id_t getBlockCount( const llvm::BasicBlock* b  );
    // void initBlockCount( llvm::Function &f, size_t threadId );

    // z3:expr getPhiMap( const llvm::PHINode*, StringVec, ValueExprMap& );

    // z3:expr
    // getPhiMap( const llvm::PHINode* p, ValueExprMap& m );

    z3::expr translateBlock( const llvm::BasicBlock*,
                             const hb_enc::se_set& prev_events,
                             std::map<llvm::BasicBlock*,z3::expr>& );

    // void post_insertEdge( unsigned, unsigned, z3:expr );

    // void resetPendingInsertEdge();
    // void addPendingInsertEdge( unsigned, unsigned, z3:expr);
    // void applyPendingInsertEdges( unsigned );

    z3::expr getTerm( const llvm::Value* op ,ValueExprMap& m ) {
    if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
      unsigned bw = c->getBitWidth();
      if(bw > 1) {
        int i = readInt(c);
        return z3.c.int_val(i);
      }else if(bw == 1) {
        bool i = c->getType();
	return z3.c.bool_val(i);
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
      const llvm::APFloat& n = c->getValueAPF();
      double v = n.convertToDouble();
      //return z3.c.real_val(v);
    }else if( auto c = llvm::dyn_cast<llvm::ConstantExpr>(op) ) {
    }else if( auto c = llvm::dyn_cast<llvm::ConstantArray>(op) ) {
      const llvm::ArrayType* n = c->getType();
      unsigned len = n->getNumElements();
      //return z3.c.arraysort();
    }else if( auto c = llvm::dyn_cast<llvm::ConstantStruct>(op) ) {
      const llvm::StructType* n = c->getType();
    }else if( auto c = llvm::dyn_cast<llvm::ConstantVector>(op) ) {
      const llvm::VectorType* n = c->getType();
      }else{
	 auto it = m.find( op );
         if( it == m.end() ) {
           llvm::outs() << "\n";
	   std::cerr << "local term not found!";
	 }
         return it->second;
	}
    }

    // bool isValueMapped( const llvm::Value* op ,ValueExprMap& m ) {
    //   if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
    //   }else if( !eHandler->isLocalVar( op ) ) {
    //     auto it = m.find( op );
    //     if( it == m.end() )
    //       return false;
    //   }
    //   return true;
    // }
     bool isLocalVar( const llvm::Value* g  ) {
      auto it = localVars.find( g );
      if( it == localVars.end() ) return false;
      return true;
    }

    int readInt( const llvm::ConstantInt* c ) {
      const llvm::APInt& n = c->getUniqueInteger();
      unsigned len = n.getNumWords();
      if( len > 1 ) std::cerr << "long integers not supported!!";
      const uint64_t *v = n.getRawData();
      return *v;
   }
  };

}}
#endif // TARA_CINPUT_BUILD_PROGRAM_H
