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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/Support/raw_ostream.h"
#pragma GCC diagnostic pop

namespace tara {
namespace cinput {

  class build_program : public llvm::FunctionPass {

  public:
    typedef std::map< const llvm::Value*, z3::expr > ValueExprMap;
    static char ID;

    //SimpleMultiThreadedProgram<z3:expr>::location_id_type program_location_id_t;
    // SimpleMultiThreadedProgram<z3:expr>::thread_id_type thread_id_t;

  private:
    helpers::z3interf& z3;
    // Cfrontend::Config& config;
    program* p;
    unsigned inst_counter = 0;
    unsigned thread_count = 0;

  public:
    build_program( helpers::z3interf& z3_,
                   // Cfrontend::Config& config_,
                   program* program_ );

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

    z3::expr translateBlock( llvm::BasicBlock*, z3::expr , ValueExprMap& );

    // void post_insertEdge( unsigned, unsigned, z3:expr );

    // void resetPendingInsertEdge();
    // void addPendingInsertEdge( unsigned, unsigned, z3:expr);
    // void applyPendingInsertEdges( unsigned );

    // z3:expr getTerm( const llvm::Value* op ,ValueExprMap& m ) {
    //   if( const llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(op) ) {
    //     int i = readInt( c );
    //     return eHandler->mkIntVal( i );
    //   }else if( auto c = llvm::dyn_cast<llvm::ConstantPointerNull>(op) ) {
    //   // }else if( LLCAST( ConstantPointerNull, c, op) ) {
    //     return eHandler->mkIntVal( 0 );
    //   }else if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
    //     cfrontend_error( "un recognized constant!" );
    //     // int i = readInt(c);
    //     // return eHandler->mkIntVal( i );
    //   }else if( eHandler->isLocalVar( op ) ) {
    //     return eHandler->getLocalVar( op );
    //   }else{
    //     auto it = m.find( op );
    //     if( it == m.end() ) {
    //       op->print( llvm::outs() ); //llvm::outs() << "\n";
    //       cfrontend_error( "local term not found!" );
    //     }
    //     return it->second;
    //   }
    // }

    // bool isValueMapped( const llvm::Value* op ,ValueExprMap& m ) {
    //   if( const llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(op) ) {
    //   }else if( !eHandler->isLocalVar( op ) ) {
    //     auto it = m.find( op );
    //     if( it == m.end() )
    //       return false;
    //   }
    //   return true;
    // }

  };

}}
#endif // TARA_CINPUT_BUILD_PROGRAM_H
