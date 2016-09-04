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
#include "helpers/z3interf.h"
#include "api/options.h"
#include "helpers/helpers.h"
#include "program/program.h"
#include "cinput/cinput.h"
#include "cinput/cinput_exception.h"
#include <utility>
#include "hb_enc/symbolic_event.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Pass.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#pragma GCC diagnostic pop

namespace tara {
namespace cinput {

  //----------------------------------
  // should be moved to its own file
  z3::sort llvm_to_z3_sort( z3::context& c, llvm::Type* t );
  std::string getInstructionLocationName(const llvm::Instruction* I );
  void initBlockCount( llvm::Function &f,
                       std::map<const llvm::BasicBlock*, unsigned>& block_to_id);
  void removeBranchingOnPHINode( llvm::BranchInst *branch );
  //-------------------------------------------------------

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
    void print(std::ostream& os) const;
    // friend bool operator==(const split_step& step1, const split_step& step2);
  };

  typedef std::vector<split_step> split_history;
  void print( std::ostream& os, const split_history&  hs );

  class SplitAtAssumePass : public llvm::FunctionPass {
  public:
    static char ID;
    SplitAtAssumePass() : llvm::FunctionPass(ID) {}
    virtual bool runOnFunction( llvm::Function &f );
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const;
  };

  // conditional pointing; conditions may overlap due to non-determinism
  typedef std::vector< std::pair< z3::expr, cssa::variable > > points_set_t;

  class build_program : public llvm::FunctionPass {

  public:
    typedef std::map< const llvm::Value*, z3::expr > ValueExprMap;
    ValueExprMap m;
    std::map< const llvm::Value*, cssa::variable > localVars;
    typedef std::map< const llvm::Value*, hb_enc::depends_set > local_data_dependency;
    typedef std::map< const llvm::BasicBlock*, hb_enc::depends_set > local_ctrl_dependency;
    static char ID;
    std::string name;
    //SimpleMultiThreadedProgram<z3:expr>::location_id_type program_location_id_t;
    // SimpleMultiThreadedProgram<z3:expr>::thread_id_type thread_id_t;

  private:
    helpers::z3interf& z3;
    api::options& o;
    hb_enc::encoding& hb_encoding;
    // Cfrontend::Config& config;
    tara::program* p;
    unsigned inst_counter = 0;
    ValueExprMap valMap;
    std::map< const llvm::BasicBlock*, unsigned> block_to_id;
    std::map< const llvm::BasicBlock*, split_history > block_to_split_stack;
    std::map< const llvm::BasicBlock*, z3::expr > block_to_exit_bit;
    std::map< llvm::BasicBlock*, hb_enc::se_set>  block_to_trailing_events;

    //-----------------------------------
    // local data structure of dependency
    //hb_enc::depends_set data_dep_ses;
    hb_enc::depends_set ctrl_dep_ses;
    build_program::local_data_dependency local_map;
    build_program::local_ctrl_dependency local_ctrl;
    //-----------------------------------

    class pointing {
    public:
      pointing( points_set_t p_set_, z3::expr null_cond_ )
        : p_set(p_set_) { null_cond.insert( null_cond_ ); }
      pointing( points_set_t p_set_ ) : p_set(p_set_) {}
      points_set_t p_set;
      std::set<z3::expr> null_cond;
      bool has_null() { return !null_cond.empty(); }
      z3::expr get_null_cond() {
        assert( null_cond.size() == 1 );
        return *null_cond.begin();
      }
    };
    std::map< const llvm::Value*, pointing > points_to;

    //thread creation map
    std::map< const llvm::Value*, std::string > ptr_to_create;

    //
    std::map< llvm::BasicBlock*, z3::expr > block_to_path_con;
    z3::expr phi_instr = z3.mk_true();
    z3::expr phi_cond = z3.mk_true();

    void join_histories( const std::vector<llvm::BasicBlock*>& preds,
                         const std::vector<split_history>& hs,
                         split_history& h,
                         z3::expr& path_cond,
                         std::map<const llvm::BasicBlock*,z3::expr>& conds
                         );

   hb_enc::depends_set get_depends( const llvm::Value* op );
   hb_enc::depends_set get_ctrl( const llvm::BasicBlock* b);
   z3::expr getPhiMap ( const llvm::Value* op, ValueExprMap& m );
  public:
    build_program( helpers::z3interf& z3_,
                   api::options& o_,
                   hb_enc::encoding& hb_encoding_,
                   tara::program* program_ )
    : llvm::FunctionPass(ID)
    , z3(z3_)
    , o(o_)
    , hb_encoding( hb_encoding_ )
    , p( program_ )
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

    z3::expr translateBlock( unsigned thr_id,
                             const llvm::BasicBlock*,
                             hb_enc::se_set& prev_events,
                             z3::expr& path_cond,
                             std::vector< z3::expr >& history,
                             std::map<const llvm::BasicBlock*,z3::expr>& conds );

    // void post_insertEdge( unsigned, unsigned, z3:expr );

    // void resetPendingInsertEdge();
    // void addPendingInsertEdge( unsigned, unsigned, z3:expr);
    // void applyPendingInsertEdges( unsigned );

    z3::expr getTerm( const llvm::Value* op ,ValueExprMap& m ) {
      return get_term( z3, op, m );
    }

    static void insert_term_map( const llvm::Value* op ,
                                 z3::expr e, ValueExprMap& m ) {
      auto pair = std::make_pair( op, e );
      m.insert( pair );
      // m.at(op) = e;
      // implement this
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

    static int readInt( const llvm::ConstantInt* c ) {
      const llvm::APInt& n = c->getUniqueInteger();
      unsigned len = n.getNumWords();
      if( len > 1 ) cinput_error( "long integers not supported!!" );
      const uint64_t *v = n.getRawData();
      return *v;
    }

  public:
    static z3::expr get_term( helpers::z3interf& z3_,
                              const llvm::Value* op ,ValueExprMap& m );
    // static z3::expr fresh_int( helpers::z3interf& z3_ );
    // static z3::expr fresh_bool( helpers::z3interf& z3_ );

  };

}}
#endif // TARA_CINPUT_BUILD_PROGRAM_H
