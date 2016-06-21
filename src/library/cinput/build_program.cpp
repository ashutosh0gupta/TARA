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

//----------------------------------------------------------------------------
// code for input object generation

build_program::build_program( helpers::z3interf& z3_,
                              api::options& o_,
                              program* program_ )
  : llvm::FunctionPass(ID)
  , z3(z3_)
  , o(o_)
  , p( program_ )
  , thread_count( 0 )
{}


z3::expr build_program::translateBlock( llvm::BasicBlock* b,
                                         z3::expr path_cons,
                                         ValueExprMap& m ) {
  // assert(b);
  // std::vector<typename EHandler::expr> blockTerms;
  // //iterate over instructions
  for( llvm::Instruction& Iobj : b->getInstList() ) {
    llvm::Instruction* I = &(Iobj);
  //   assert( I );
  //   typename EHandler::expr term = eHandler->mkEmptyExpr();
  //   bool recognized = false, record = false;
    if( const llvm::StoreInst* str = llvm::dyn_cast<llvm::StoreInst>(I) ) {
      llvm::Value* v = str->getOperand(0);
      llvm::Value* g = str->getOperand(1);
  //     term = getTerm( v, m );
  //     typename EHandler::expr gp     = eHandler->getGlobalVarNext( g );
  //     typename EHandler::expr assign = eHandler->mkEq( gp, term );
  //     blockTerms.push_back( assign );
  //     assert( !recognized );recognized = true;
    }
    if( const llvm::BinaryOperator* bop =
        llvm::dyn_cast<llvm::BinaryOperator>(I) ) {
      llvm::Value* op0 = bop->getOperand( 0 );
      llvm::Value* op1 = bop->getOperand( 1 );
  //     typename EHandler::expr o0 = getTerm( op0, m );
  //     typename EHandler::expr o1 = getTerm( op1, m );
      unsigned op = bop->getOpcode();
  //     switch( op ) {
  //     case llvm::Instruction::Add : term = eHandler->mkPlus ( o0, o1 ); break;
  //     case llvm::Instruction::Sub : term = eHandler->mkMinus( o0, o1 ); break;
  //     case llvm::Instruction::Mul : term = eHandler->mkMul  ( o0, o1 ); break;
  //     case llvm::Instruction::Xor : term = eHandler->mkXor  ( o0, o1 ); break;
  //       // case llvm::Instruction::SDiv: term = eHandler->mkSDiv ( o0, o1 ); break;
  //     default: {
  //       const char* opName = bop->getOpcodeName();
  //       cfrontend_error( "unsupported instruction " << opName
  //                        << " occurred!!"  );
  //     }
  //     }
  //     record = true;
  //     assert( !recognized );recognized = true;
    }
    if( const llvm::UnaryInstruction* str =
        llvm::dyn_cast<llvm::UnaryInstruction>(I) ) {
  //     if( const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(I) ) {
  //       llvm::Value* g = load->getOperand(0);
  //       term = eHandler->getGlobalVar(g);
  //       record = true;
  //       assert( !recognized );recognized = true;
  //     }else{
  //       assert( false );
  //       cfrontend_warning( "I am uniary instruction!!" );
  //       assert( !recognized );recognized = true;
  //     }
    }
    if( const llvm::CmpInst* cmp = llvm::dyn_cast<llvm::CmpInst>(I) ) {
  //     llvm::Value* lhs = cmp->getOperand( 0 );
  //     llvm::Value* rhs = cmp->getOperand( 1 );
  //     typename EHandler::expr l = getTerm( lhs, m );
  //     typename EHandler::expr r = getTerm( rhs, m );
  //     llvm::CmpInst::Predicate pred = cmp->getPredicate();
  //     switch( pred ) {
  //     case llvm::CmpInst::ICMP_EQ  : term = eHandler->mkEq ( l, r ); break;
  //     case llvm::CmpInst::ICMP_NE  : term = eHandler->mkNEq( l, r ); break;
  //     case llvm::CmpInst::ICMP_UGT : term = eHandler->mkGT ( l, r ); break;
  //     case llvm::CmpInst::ICMP_UGE : term = eHandler->mkGEq( l, r ); break;
  //     case llvm::CmpInst::ICMP_ULT : term = eHandler->mkLT ( l, r ); break;
  //     case llvm::CmpInst::ICMP_ULE : term = eHandler->mkLEq( l, r ); break;
  //     case llvm::CmpInst::ICMP_SGT : term = eHandler->mkGT ( l, r ); break;
  //     case llvm::CmpInst::ICMP_SGE : term = eHandler->mkGEq( l, r ); break;
  //     case llvm::CmpInst::ICMP_SLT : term = eHandler->mkLT ( l, r ); break;
  //     case llvm::CmpInst::ICMP_SLE : term = eHandler->mkLEq( l, r ); break;
  //     default: {
  //       cfrontend_error( "unsupported predicate in compare instruction"
  //                        << pred << "!!"  );
  //     }
  //     }
  //     record = true;
  //     assert( !recognized );recognized = true;
    }
  //   if( const llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(I) ) {
  //     term = getPhiMap( phi, m);
  //     record = 1;
  //     assert( !recognized );recognized = true;
  //   }
  //   if( auto ret = llvm::dyn_cast<llvm::ReturnInst>(I) ) {
  //     llvm::Value* v = ret->getReturnValue();
  //     if( v ) {
  //       typename EHandler::expr retTerm = getTerm( v, m );
  //       //todo: use retTerm somewhere
  //     }
  //     if( config.verbose("mkthread") )
  //       cfrontend_warning( "return value ignored!!" );
  //     assert( !recognized );recognized = true;
  //   }
  //   if( auto unreach = llvm::dyn_cast<llvm::UnreachableInst>(I) ) {
  //     if( config.verbose("mkthread") )
  //       cfrontend_warning( "unreachable instruction ignored!!" );
  //     assert( !recognized );recognized = true;
  //   }

  //   // UNSUPPORTED_INSTRUCTIONS( ReturnInst,      I );
  //   UNSUPPORTED_INSTRUCTIONS( InvokeInst,      I );
  //   UNSUPPORTED_INSTRUCTIONS( IndirectBrInst,  I );
  //   // UNSUPPORTED_INSTRUCTIONS( UnreachableInst, I );

    if( const llvm::BranchInst* br = llvm::dyn_cast<llvm::BranchInst>(I) ) {
  //     if( br->isConditional() ) {
  //       llvm::Value* c = br->getCondition();
  //       typename EHandler::expr cond = getTerm( c, m);
  //       if( succ == 0 ) {
  //         blockTerms.push_back( cond );
  //       }else{
  //         typename EHandler::expr notCond = eHandler->mkNot( cond );
  //         blockTerms.push_back( notCond );
  //       }
  //     }else{cfrontend_warning( "I am unconditional branch!!" );}
  //     assert( !recognized );recognized = true;
    }
    if( const llvm::SwitchInst* t = llvm::dyn_cast<llvm::SwitchInst>(I) ) {
  //     cfrontend_warning( "I am switch!!" );
  //     assert( !recognized );recognized = true;
    }
  //   if( const llvm::CallInst* str = llvm::dyn_cast<llvm::CallInst>(I) ) {
  //     if( const llvm::DbgValueInst* dVal =
  //         llvm::dyn_cast<llvm::DbgValueInst>(I) ) {
  //       // Ignore debug instructions
  //     }else{ cfrontend_warning( "I am caller!!" ); }
  //     assert( !recognized );recognized = true;
  //   }
  //   // Store in term map the result of current instruction
  //   if( record ) {
  //     if( eHandler->isLocalVar( I ) ) {
  //       typename EHandler::expr lp     = eHandler->getLocalVarNext( I );
  //       typename EHandler::expr assign = eHandler->mkEq( lp, term );
  //       blockTerms.push_back( assign );
  //     }else{
  //       const llvm::Value* v = I;
  //       eHandler->insertMap( v, term, m);
  //     }
  //   }
  //   if( !recognized ) cfrontend_error( "----- failed to recognize!!" );
  }
  // // print_double_line( std::cout );
  // //iterate over instructions and build
  return z3.c.bool_val(true); //eHandler->mkAnd( blockTerms );
}

bool build_program::runOnFunction( llvm::Function &f ) {
  thread_count++;
  std::string name = (std::string)f.getName();
  unsigned threadId = p->add_thread( name );

  initBlockCount( f, block_to_id );

  // collect local variables

  //iterate over debug instructions

  // // TODO: remove nameExprMap from to avoid ugliness of accessing
  // // maps with z3 expressions
  // std::map< std::string, typename EHandler::expr> nameExprMap;
  // std::map< std::string, typename EHandler::expr> nameNextExprMap;
  // std::map<const llvm::Value*, std::string> localNameMap;
  // localNameMap.clear();
  // buildLocalNameMap( f, localNameMap );
  // eHandler->resetLocalVars();
  // for(auto it=localNameMap.begin(), end = localNameMap.end(); it!=end;++it) {
  //   const llvm::Value* v = it->first;
  //   std::string name = it->second;
  //   std::string nameNext = name + "__p";
  //   if( config.verbose("mkthread") ) {
  //     std::cout << "local variable discovered: "<< name << "\n";
  //   }
  //   if( nameExprMap.find(name) == nameExprMap.end() ) {
  //     typename EHandler::expr lv  = eHandler->mkVar( name  );
  //     typename EHandler::expr lvp = eHandler->mkVar( nameNext );
  //     program->addLocal( threadId, lv, lvp );
  //     // TODO: why the following code does not work with z3 expr
  //     auto p = std::pair<std::string,typename EHandler::expr>( name, lv );
  //     nameExprMap.insert(p); // nameExprMap[name] = lv;
  //     auto n = std::pair<std::string,typename EHandler::expr>(nameNext,lvp);
  //     nameNextExprMap.insert(n); // nameNextExprMap[nameNext] = lvNext;
  //   }
  //   auto curr_it = nameExprMap.find(name);
  //   auto next_it = nameNextExprMap.find(nameNext);
  //   eHandler->addLocalVar( v, curr_it->second, next_it->second );
  // }

  // ValueExprMap exprMap;

  // resetPendingInsertEdge();

  for( auto it = f.begin(), end = f.end(); it != end; it++ ) {
    llvm::BasicBlock* src = &(*it);
    if( o.print_input > 0 ) src->print( llvm::outs() );
    // z3::expr b =  z3.c.bool_val(false);
    // auto PI = llvm::pred_begin(src);
    // split_history h;
    // if( PI != pred_end(src) ) {
    //   llvm::BasicBlock *prev = *PI++;
    //   const split_history& h1 = block_to_split_stack[prev];
    //   // PI++;
    //   if( PI != pred_end(src) ) {// two predecessor
    //     const split_history& h2 = block_to_split_stack[*PI++];
    //     // join_histories( h1, h2, h );
    //     if(  PI != pred_end(src) ) tara_error( "cinput : more than two preds!" );
    //   }else{ // one predecessor
    //     h = h1;
    //   }
    // }

    std::vector< split_history > histories;
    for( auto PI = llvm::pred_begin(src), E = llvm::pred_end(src);PI != E;++PI){
      llvm::BasicBlock *prev = *PI;
      split_history h = block_to_split_stack[prev];
      if( llvm::isa<llvm::BranchInst>( prev->getTerminator() ) ) {
        z3::expr& b = block_to_exit_bit.at(prev); // todo: it may not exist
        llvm::TerminatorInst *term= prev->getTerminator();
        unsigned succ_num = PI.getOperandNo();
        if( succ_num == 1 ) b = !b;
        // h.push_back( prev, block_to_id[prev], succ_num, b);
      }
      histories.push_back(h);
    }
    if( histories.size() == 0 ) {
      
    }else{

    }

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
